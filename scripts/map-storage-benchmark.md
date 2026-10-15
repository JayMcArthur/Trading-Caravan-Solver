# Search Map Storage Benchmark

This is a read-only experiment for original maps, not an archive/apply tool.
Run it only with map writers stopped. It copies the complete SQLite main, WAL
and SHM set into a disposable private directory and checks source fingerprints
before and after. All SQLite opens, checkpointing on close and VACUUM INTO run
against those disposable copies, never the original.

Inside the existing locked trading_caravan shell:

```bash
LIMIT_MAX_MB=512 LIMIT_MAX_SECONDS=180 ./scripts/run_limited.sh python3 \
  scripts/benchmark_map_storage.py /absolute/map.sqlite3 \
  --output /private/new-result.json

LIMIT_MAX_MB=512 LIMIT_MAX_SECONDS=180 ./scripts/run_limited.sh python3 \
  -m unittest discover -s tests -p MapStorageBenchmarkTests.py -v
```

The result records complete gzip archive size/restore time and the size of a
normally readable repacked SQLite file. Every archived file is independently
restored and SHA256-checked before SQLite opens it. Original snapshot, archive
restore and repacked copy must all pass integrity_check and match every logical
table row, explicit key, value and schema definition. Tables without explicit
primary keys are rejected: VACUUM may change implicit rowids. The live maps have
explicit application keys; compare them rather than relying on physical layout.

Temporary archives, restored files and repacked copies are deleted at completion;
only the requested result JSON remains. No measured disk saving is applied to
originals. Interrupted probes may leave disposable temporary files for inspection.
Do not extrapolate two small-map ratios into guaranteed fleet-wide savings.

Cold archives require restoration before ordinary SQLite use. Simply removing a
map is unsafe: the current graph constructor permits SQLITE_OPEN_CREATE and could
silently start an empty graph. A real archive workflow must prevent that across
old clients and provide deliberate, bounded, atomic restore before any cutover.
