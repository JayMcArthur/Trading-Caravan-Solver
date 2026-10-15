#!/usr/bin/env python3
"""Preview or recoverably replace dense2 purchase caches with existing sparse3."""

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import stat
import struct
import subprocess
import tempfile


def digest(path, compressed=False):
    with (gzip.open(path, 'rb') if compressed else path.open('rb')) as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def identity(path):
    info = path.stat()
    return info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns


def sync_directory(path):
    descriptor = os.open(path, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def receipt(path, result):
    with tempfile.NamedTemporaryFile(mode='w', dir=path.parent, delete=False) as output:
        json.dump(result, output, indent=2)
        output.write('\n')
        output.flush()
        os.fsync(output.fileno())
    os.replace(output.name, path)
    sync_directory(path.parent)


def convert(source, binary, recovery, apply=False):
    source, binary, recovery = Path(source).absolute(), Path(binary).absolute(), Path(recovery).absolute()
    if source.is_symlink() or not source.is_file():
        raise ValueError('Cache must be a regular file, not a symlink')
    before = identity(source)
    with source.open('rb') as stream:
        header = stream.read(16)
    if len(header) < 8:
        raise ValueError('Truncated purchase cache')
    version = struct.unpack_from('<I', header)[0]
    result = {'source': str(source), 'original_bytes': before[2],
              'original_mode': stat.S_IMODE(source.stat().st_mode),
              'original_mtime_ns': before[3], 'applied': False}
    if version == 3:
        return {**result, 'action': 'already_sparse'}
    if version != 2 or len(header) != 16:
        raise ValueError('Expected dense2 or sparse3 purchase cache')
    _, gold, weight, count = struct.unpack('<IHHQ', header)
    if gold == 65535 or weight == 65535 or count != (gold + 1) * (weight + 1) or before[2] != 16 + count * 10:
        raise ValueError('Invalid or unsupported dense cache size')
    result.update(action='convert', cells=count)
    if not apply:
        return result
    original_hash = digest(source)
    if identity(source) != before:
        raise ValueError('Source changed while hashing')
    if recovery.is_symlink():
        raise ValueError('Recovery directory must not be a symlink')
    recovery.mkdir(parents=True, exist_ok=True, mode=0o700)
    if recovery.stat().st_uid != os.getuid() or stat.S_IMODE(recovery.stat().st_mode) != 0o700:
        raise ValueError('Recovery directory must be owned by you with mode700')
    backup = recovery / (source.name + '.' + original_hash + '.gz')
    if backup.is_symlink():
        raise ValueError('Recovery file must not be a symlink')
    if not backup.exists():
        descriptor = os.open(backup, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(descriptor, 'wb') as raw:
            with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0, compresslevel=9) as packed:
                with source.open('rb') as stream:
                    while block := stream.read(1024 * 1024):
                        packed.write(block)
            raw.flush()
            os.fsync(raw.fileno())
        sync_directory(recovery)
    if digest(backup, compressed=True) != original_hash:
        raise ValueError('Original recovery verification failed; source retained')
    with tempfile.TemporaryDirectory(prefix='.cache-convert-', dir=source.parent) as temporary:
        output = Path(temporary) / source.name
        completed = subprocess.run([str(binary), str(source), str(output)], check=True,
                                   text=True, stdout=subprocess.PIPE)
        verified = json.loads(completed.stdout)
        if verified['cells_verified'] != count or verified['rows_verified'] != weight + 1:
            raise ValueError('Native equivalence evidence does not match source')
        if output.stat().st_size >= before[2]:
            return {**result, 'action': 'not_smaller', 'backup': str(backup)}
        if identity(source) != before or digest(source) != original_hash:
            raise ValueError('Source changed during conversion; no replacement')
        output.chmod(stat.S_IMODE(source.stat().st_mode))
        with output.open('rb') as stream:
            os.fsync(stream.fileno())
        result.update(original_sha256=original_hash, converted_sha256=digest(output),
                      converted_bytes=output.stat().st_size, backup=str(backup),
                      backup_sha256=digest(backup), native_verification=verified, status='pending_replace')
        record = recovery / (backup.name + '.json')
        receipt(record, result)
        if identity(source) != before:
            result['status'] = 'source_changed'
            receipt(record, result)
            raise ValueError('Source changed before replacement; retained')
        os.replace(output, source)
        sync_directory(source.parent)
        result.update(applied=True, status='converted')
        receipt(record, result)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('files', nargs='+', type=Path)
    parser.add_argument('--binary', type=Path, default=Path(__file__).resolve().parents[1] / 'build/ConvertPurchaseCache')
    parser.add_argument('--recovery', type=Path, default=Path.home() / '.local/state/trading-caravan/cache-conversion')
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    for source in args.files:
        print(json.dumps(convert(source, args.binary, args.recovery, args.apply)), flush=True)


if __name__ == '__main__':
    main()
