import gzip
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('converter', ROOT / 'scripts/convert_purchase_cache.py')
converter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(converter)
BINARY = ROOT / 'build/ConvertPurchaseCache'


def dense(path):
    data = struct.pack('<IHHQ', 2, 4, 2, 15)
    for gold in range(5):
        for weight in range(3):
            # Equal-profit cells deliberately change buys; match existing reader tie handling.
            data += struct.pack('<I6B', (gold // 2) * 5, gold, weight, 0, 0, 0, 0)
    path.write_bytes(data)
    return data


class CacheConversionTests(unittest.TestCase):
    def test_preview_no_writes_and_native_conversion_restores_exact_original(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path, recovery = root / 'tree.bin', root / 'recovery'
            original = dense(path)
            path.chmod(0o640)
            self.assertFalse(converter.convert(path, BINARY, recovery)['applied'])
            self.assertFalse(recovery.exists())
            result = converter.convert(path, BINARY, recovery, apply=True)
            self.assertTrue(result['applied'])
            self.assertEqual(result['native_verification'], {'cells_verified': 15, 'rows_verified': 3})
            self.assertEqual(struct.unpack('<I', path.read_bytes()[:4])[0], 3)
            self.assertEqual(path.stat().st_mode & 0o777, 0o640)
            self.assertEqual(gzip.decompress(Path(result['backup']).read_bytes()), original)
            self.assertEqual(converter.digest(path), result['converted_sha256'])
            before = path.read_bytes()
            self.assertEqual(converter.convert(path, BINARY, recovery, True)['action'], 'already_sparse')
            self.assertEqual(path.read_bytes(), before)
            records = list(recovery.glob('*.json'))
            self.assertEqual(len(records), 1)
            self.assertEqual(json.loads(records[0].read_text())['status'], 'converted')

    def test_corrupt_input_and_failed_worker_leave_source_unchanged(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path, recovery = root / 'tree.bin', root / 'recovery'
            original = dense(path)
            for data in [original[:-1], original + b'extra', struct.pack('<I', 99)]:
                path.write_bytes(data)
                with self.assertRaises(ValueError):
                    converter.convert(path, BINARY, recovery, True)
                self.assertEqual(path.read_bytes(), data)
                self.assertFalse(recovery.exists())
            path.write_bytes(original)
            with self.assertRaises(subprocess.CalledProcessError):
                converter.convert(path, Path(shutil.which('false')), recovery, True)
            self.assertEqual(path.read_bytes(), original)
            self.assertEqual(gzip.decompress(next(recovery.glob('*.gz')).read_bytes()), original)
            self.assertFalse(list(root.glob('.cache-convert-*')))

    def test_native_converter_refuses_existing_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path, output = root / 'tree.bin', root / 'retained.bin'
            dense(path)
            output.write_bytes(b'retain existing bytes')
            result = subprocess.run([str(BINARY), str(path), str(output)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), b'retain existing bytes')

    def test_concurrent_edit_at_receipt_is_not_overwritten(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path, recovery = root / 'tree.bin', root / 'recovery'
            original = dense(path)
            write_receipt = converter.receipt
            def edited(record, result):
                write_receipt(record, result)
                if result['status'] == 'pending_replace':
                    path.write_bytes(b'new work from another writer')
            with patch.object(converter, 'receipt', side_effect=edited):
                with self.assertRaisesRegex(ValueError, 'changed before replacement'):
                    converter.convert(path, BINARY, recovery, True)
            self.assertEqual(path.read_bytes(), b'new work from another writer')
            self.assertEqual(gzip.decompress(next(recovery.glob('*.gz')).read_bytes()), original)
            self.assertEqual(json.loads(next(recovery.glob('*.json')).read_text())['status'], 'source_changed')

    def test_invalid_recovery_refuses_replacement(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            path, recovery = root / 'tree.bin', root / 'recovery'
            original = dense(path)
            recovery.mkdir(mode=0o700)
            backup = recovery / (path.name + '.' + converter.digest(path) + '.gz')
            backup.write_bytes(b'not an original gzip archive')
            with self.assertRaises(OSError):
                converter.convert(path, BINARY, recovery, True)
            self.assertEqual(path.read_bytes(), original)
