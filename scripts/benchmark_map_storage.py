#!/usr/bin/env python3
"""Measure whole-map archives and VACUUM INTO using disposable copies only."""

import argparse
from contextlib import closing
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import sqlite3
import struct
import tarfile
import tempfile
import time


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def original_files(source):
    return [p for p in (source, Path(str(source) + '-wal'), Path(str(source) + '-shm')) if p.exists()]


def fingerprint(source):
    result = {}
    for path in original_files(source):
        if path.is_symlink() or not path.is_file():
            raise ValueError('Original must be a regular file')
        s = path.stat()
        result[path.name] = {'bytes': s.st_size, 'inode': s.st_ino, 'mode': s.st_mode,
                             'mtime_ns': s.st_mtime_ns, 'ctime_ns': s.st_ctime_ns,
                             'sha256': digest(path)}
    return result


def quote(name):
    return '"' + name.replace('"', '""') + '"'


def logical_content(connection):
    schema = connection.execute("SELECT type,name,tbl_name,sql FROM sqlite_schema WHERE name NOT LIKE 'sqlite_%' ORDER BY type,name").fetchall()
    tables = {}
    for _, name, _, _ in schema:
        if not connection.execute("SELECT 1 FROM sqlite_schema WHERE type='table' AND name=?", (name,)).fetchone():
            continue
        columns = connection.execute('PRAGMA table_info(' + quote(name) + ')').fetchall()
        keys = [column[1] for column in sorted(columns, key=lambda c: c[5]) if column[5]]
        if not keys:
            raise ValueError('Benchmark needs explicit primary keys: ' + name)
        h = hashlib.sha256()
        count = 0
        query = 'SELECT * FROM ' + quote(name) + ' ORDER BY ' + ','.join(map(quote, keys))
        for row in connection.execute(query):
            count += 1
            for value in row:
                if value is None:
                    h.update(b'n')
                elif isinstance(value, int):
                    h.update(b'i' + struct.pack('<q', value))
                elif isinstance(value, float):
                    h.update(b'f' + struct.pack('<d', value))
                else:
                    data = value if isinstance(value, bytes) else value.encode('utf-8')
                    h.update((b'b' if isinstance(value, bytes) else b's') + struct.pack('<Q', len(data)))
                    h.update(data)
        tables[name] = {'rows': count, 'sha256': h.hexdigest()}
    return {'schema': schema, 'tables': tables}


def verify_database(path):
    with closing(sqlite3.connect(path)) as connection:
        connection.execute('PRAGMA cache_size=-2048')
        connection.execute('PRAGMA temp_store=FILE')
        if connection.execute('PRAGMA integrity_check').fetchall() != [('ok',)]:
            raise ValueError('Disposable database integrity check failed')
        return logical_content(connection)


def benchmark(source):
    source = Path(source).resolve(strict=True)
    before = fingerprint(source)
    started = time.monotonic()
    with tempfile.TemporaryDirectory(prefix='caravan-map-storage-') as temporary:
        root = Path(temporary)
        copied, restored = root / 'copy', root / 'restore'
        copied.mkdir()
        restored.mkdir()
        for path in original_files(source):
            target = copied / path.name
            shutil.copyfile(path, target)
            if digest(target) != before[path.name]['sha256']:
                raise ValueError('Copy differs from original')
        if fingerprint(source) != before:
            raise ValueError('Original changed during copy; benchmark refused')
        archive = root / 'map.tar.gz'
        packing = time.monotonic()
        with archive.open('xb') as raw:
            with gzip.GzipFile(filename='', fileobj=raw, mode='wb', compresslevel=6, mtime=0) as packed:
                with tarfile.open(fileobj=packed, mode='w|') as bundle:
                    for name in before:
                        bundle.add(copied / name, arcname=name)
        packed_seconds = time.monotonic() - packing
        unpacking = time.monotonic()
        with tarfile.open(archive, 'r:gz') as bundle:
            names = set()
            for member in bundle:
                if member.name not in before or not member.isfile() or member.name in names:
                    raise ValueError('Unexpected archive member')
                names.add(member.name)
                with bundle.extractfile(member) as stream, (restored / member.name).open('xb') as target:
                    shutil.copyfileobj(stream, target)
            if names != set(before):
                raise ValueError('Archive is incomplete')
        for name, item in before.items():
            if digest(restored / name) != item['sha256']:
                raise ValueError('Restored original bytes differ')
        restored_seconds = time.monotonic() - unpacking
        baseline = verify_database(copied / source.name)
        if verify_database(restored / source.name) != baseline:
            raise ValueError('Archive changed logical data')
        vacuumed = root / 'repacked.sqlite3'
        repacking = time.monotonic()
        with closing(sqlite3.connect(copied / source.name)) as connection:
            connection.execute('PRAGMA cache_size=-2048')
            connection.execute('PRAGMA temp_store=FILE')
            connection.execute('PRAGMA synchronous=FULL')
            connection.execute('VACUUM INTO ?', (str(vacuumed),))
        repacked_seconds = time.monotonic() - repacking
        if verify_database(vacuumed) != baseline:
            raise ValueError('Repacking changed logical data or schema')
        if fingerprint(source) != before:
            raise ValueError('Original changed during benchmark')
        return {'source': str(source), 'original_file_bytes': sum(p['bytes'] for p in before.values()),
                'main_bytes': before[source.name]['bytes'], 'files_preserved': len(before),
                'archive_bytes': archive.stat().st_size, 'archive_seconds': packed_seconds,
                'restore_seconds': restored_seconds, 'vacuum_bytes': vacuumed.stat().st_size,
                'vacuum_seconds': repacked_seconds, 'logical_content': baseline,
                'source_unchanged': True, 'exact_archive_restore': True,
                'total_seconds': time.monotonic() - started}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = benchmark(args.source)
    fd = os.open(args.output, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(fd, 'w') as stream:
        json.dump(result, stream, indent=2)
    print(json.dumps({k: v for k, v in result.items() if k != 'logical_content'}))


if __name__ == '__main__':
    main()
