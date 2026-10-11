import json
import os
from pathlib import Path
import subprocess
import sys


repo = Path(__file__).resolve().parents[1]
solver_binary = Path(sys.argv[1]).with_name("Merchant_Trading_Caravan")
environment = dict(
    os.environ,
    BINARY=str(solver_binary),
    MAX_MB="384",
    INTERNAL_MB="288",
    ARRAY_LIMIT="500",
    MAX_SECONDS="30",
)


def run(*options):
    result = subprocess.run(
        [str(repo / "scripts/run_safe.sh"), "--town", "Normalia",
         "--no-auto-refine", "--benchmark-json", *options],
        cwd=repo,
        env=environment,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    return json.loads(next(line for line in reversed(result.stdout.splitlines())
                           if line.startswith("{")))


prefix = run("--incumbent-prefix-actions", "6", "--no-incumbent")
assert prefix["merchant_day"] == 0, prefix["merchant_day"]
assert prefix["witch_day"] == 0, prefix["witch_day"]
assert prefix["search_complete"]
assert prefix["best_source"] == "discovered"
assert prefix["best_score"] >= 37527, prefix["best_score"]

wrong_day = run("--merchant-day", "5", "--witch-day", "9", "--array-limit", "1")
assert wrong_day["incumbent_score"] is None, wrong_day["incumbent_score"]

matching_day = run("--merchant-day", "7", "--witch-day", "9", "--array-limit", "1")
assert matching_day["incumbent_score"] == 37527, matching_day["incumbent_score"]
print("Prefix timing and incumbent schedule regressions passed")
