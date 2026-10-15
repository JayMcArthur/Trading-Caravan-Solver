# Purchase Cache Maintenance

The current reader accepts dense version2 and sparse version3. New caches already
use version3. This offline tool converts older files without changing the reader,
lookup filenames, search maps or campaign/checkpoint data. Stop writers first;
source-stability checks are accident prevention, not a lock against other programs.

Inside the locked trading_caravan development shell:

```bash
./scripts/build_safe.sh
./scripts/run_tests_safe.sh cache-conversion
LIMIT_MAX_MB=768 LIMIT_MAX_SECONDS=600 ./scripts/run_limited.sh python3 \
  scripts/convert_purchase_cache.py /absolute/Saved_Trees/*.bin
```

Preview is the default and creates nothing. Add `--apply` to replace validated
dense files. Use `--recovery /private/directory` to select the owner-only recovery
folder; default is ~/.local/state/trading-caravan/cache-conversion. Already sparse
files are untouched. `--binary PATH` selects a separately built ConvertPurchaseCache.
The wrapper enforces the whole job budget; a timeout can stop a batch after some
completed files. Inspect receipts and rerun deliberately; no automatic retries.

Before replacement each original is SHA256-checked, gzip-compressed, fsynced and
independently decompressed/hashed. The native converter writes a new same-filesystem
temporary file, loads it using the existing reader, compares every gold/weight
query, all row frontiers and representative portfolios/clamped queries. Replacement
is atomic and happens only after the original is checked again. An invalid or
changed source, failed archive, bad output or worker failure leaves it unreplaced.
Only smaller validated outputs replace originals. No delete/archive-pruning command
is provided.

Each original archive is named BASENAME.ORIGINAL_SHA256.gz. Its JSON receipt records
original mode/mtime, both content hashes, native equivalence counts and replacement
status. Keep these compressed originals: they preserve every original byte,
including redundant equal-profit buys that the dense reader already ignores.
Recover by decompressing into a new file, checking original_sha256, then restoring
the recorded mode. Never overwrite a working file without checking current state.
A pending_replace receipt after interruption is uncertain: compare the live file
hash to its original/converted hashes before continuing; do not assume success.

This is local cache maintenance, not a new binary format, VHD compaction, campaign
or SQLite maintenance tool. Do not point it at search maps or unlink WAL/SHM files.
