"""Check the real Linux limits and nested-wrapper behavior with small jobs."""
import os
from pathlib import Path
import resource
import shutil
import subprocess
import tempfile
import time

repo = Path(__file__).resolve().parents[1]
wrapper = repo / "scripts/run_limited.sh"
scope = Path("/proc/self/cgroup").read_text().strip().split("::", 1)[1]
group = Path("/sys/fs/cgroup" + scope)
assert Path(scope).name.startswith("trading-caravan-limited-")
assert 0 < int((group / "memory.max").read_text()) <= 1024 * 1024**2
assert int((group / "memory.swap.max").read_text()) == 0
quota, period = map(int, (group / "cpu.max").read_text().split())
assert 0 < quota <= period
assert int((group / "pids.max").read_text()) == 64
assert int((group / "memory.oom.group").read_text()) == 1

environment = dict(os.environ, LIMIT_MAX_MB="256", LIMIT_MAX_SECONDS="5")
child = subprocess.run(
    [str(wrapper), "python3", "-c",
     "from pathlib import Path; import resource; "
     "print(Path('/proc/self/cgroup').read_text().strip()); "
     "print(resource.getrlimit(resource.RLIMIT_AS))"],
    env=environment, capture_output=True, text=True, check=True,
)
lines = child.stdout.splitlines()
assert lines[0] == "0::" + scope, child
assert lines[1] == str((256 * 1024**2, 256 * 1024**2)), child
assert resource.getrlimit(resource.RLIMIT_AS)[0] <= 1024 * 1024**2

failed = subprocess.run([str(wrapper), "bash", "-c", "exit 7"], env=environment)
assert failed.returncode == 7, failed
invalid = subprocess.run(
    [str(wrapper), "bash", "-c", "exit 0"],
    env=dict(environment, LIMIT_MAX_MB="1025"),
)
assert invalid.returncode == 2, invalid
with tempfile.TemporaryDirectory() as directory:
    limited_path = Path(directory)
    for name in ("bash", "prlimit", "timeout"):
        (limited_path / name).symlink_to(shutil.which(name))
    marker = limited_path / "should-not-start"
    missing_manager = subprocess.run(
        [str(wrapper), "bash", "-c", 'touch "$1"', "bash", str(marker)],
        env=dict(environment, PATH=directory),
    )
    assert missing_manager.returncode == 2, missing_manager
    assert not marker.exists()
started = time.monotonic()
expired = subprocess.run(
    [str(wrapper), "python3", "-c", "import time; time.sleep(30)"],
    env=dict(environment, LIMIT_MAX_SECONDS="1"),
)
assert expired.returncode == 124, expired
assert time.monotonic() - started < 10
print("Real cgroup limits, nested containment, exit codes and timeout passed")
