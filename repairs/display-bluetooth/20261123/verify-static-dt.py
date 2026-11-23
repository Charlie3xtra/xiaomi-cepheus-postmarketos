"""Compare full DT node/property maps and memreserve, not only selected devices."""
import sys, hashlib
from pathlib import Path
from fdt_tree import tree
a, b = (Path(x).read_bytes() for x in sys.argv[1:])
assert hashlib.sha256(a).hexdigest() == "a46645a88482f8a4a8ca5f04de18048e3267401c121218425d2ebb2b4a3b429e"
x, xr = tree(a)
y, yr = tree(b)
assert x.keys() == y.keys() and xr == yr
diff = []
for n in x:
    assert x[n].keys() == y[n].keys()
    for k in x[n]:
        if x[n][k] != y[n][k]:
            diff.append((n, k, x[n][k], y[n][k]))
assert diff == [("/soc@0/geniqup@ac0000", "status", b"disabled\0", b"okay\0")], diff
print("PASS: exactly QUP0 status changed;", len(x), "nodes; memory reservations unchanged")
