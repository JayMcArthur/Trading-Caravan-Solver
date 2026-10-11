# Native execution safety

Use the repository's bounded entry points for every C++ build or execution:

1. Build with `./scripts/build_safe.sh`.
2. Run tests with `./scripts/run_tests_safe.sh [test-selector]`.
3. Run the solver with `./scripts/run_safe.sh [solver-options]`.
4. Run any other native/debug command through
   `LIMIT_MAX_MB=768 LIMIT_MAX_SECONDS=180 ./scripts/run_limited.sh <command> [args...]`.

These commands are the completion path, including for quick checks. Direct
execution of binaries under `build/`, direct `cmake --build`, parallel warning
builds, and unbounded sanitizer/debug tools are outside the safe workflow.

When adding a native executable or Python extension:

- call `install_process_safety_limits` before application allocations;
- register automated tests through the bounded test wrapper; and
- keep compiler and linker launchers on `scripts/compiler_limiter.sh`.

Keep the enforced ceilings at or below 1,024 MB per native process and compiler
serialization enabled. Verify safety changes with shell syntax checks, the safe
build, and the safe test wrapper. A task touching native code is complete only
after those bounded checks pass.

# Search campaign budget

For record searches, spend up to 10 minutes per independent town/lane run by
default and up to 15 minutes when the added time increases state coverage.
Increase frontier or suffix retention, or run independent continuation-ranking
mixes; a longer timeout alone adds no coverage after a configuration has
completed. Run towns and ranking mixes sequentially, with every solver process
capped at 1,024 MB.

Deep-search campaign: read `benchmarks/AllTownDeepCampaign2026-09-02.md` before
tuning or launching a record sweep, including the Gayawaku-specific profile. A
campaign is complete only after its peak RSS, processed-state count,
completion/timeout status, score, and reproducible action path are recorded.
