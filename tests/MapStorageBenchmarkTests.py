import importlib.util
from pathlib import Path
import sqlite3
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('benchmark', ROOT / 'scripts/benchmark_map_storage.py')
benchmark = importlib.util.module_from_spec(spec)
spec.loader.exec_module(benchmark)


class MapStorageBenchmarkTests(unittest.TestCase):
    def test_complete_wal_restore_and_repack_preserve_every_row_and_original(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'sample.sqlite3'
            connection = sqlite3.connect(path)
            try:
                connection.execute('PRAGMA journal_mode=WAL')
                connection.execute('PRAGMA wal_autocheckpoint=0')
                connection.execute('CREATE TABLE nodes(id INTEGER PRIMARY KEY, payload BLOB, score REAL, label TEXT)')
                connection.execute('CREATE INDEX scores ON nodes(score)')
                connection.executemany('INSERT INTO nodes VALUES(?,?,?,?)', [(i, b'x' * 512, i / 3, None if i % 2 else 'name') for i in range(1000)])
                connection.commit()
                before = benchmark.fingerprint(path)
                self.assertGreater(before[path.name + '-wal']['bytes'], 0)
                result = benchmark.benchmark(path)
                self.assertTrue(result['source_unchanged'])
                self.assertTrue(result['exact_archive_restore'])
                self.assertEqual(result['logical_content']['tables']['nodes']['rows'], 1000)
                self.assertEqual(benchmark.fingerprint(path), before)
                self.assertLess(result['archive_bytes'], result['original_file_bytes'])
            finally:
                connection.close()
