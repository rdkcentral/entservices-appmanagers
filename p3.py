
from pathlib import Path

path = Path(
    "Tests/L0Tests/RuntimeManager/"
    "RuntimeManager_ComponentTests.cpp"
)

data = path.read_bytes()

old = b"auto rtCfg = MakeValidRuntimeConfiguration();"
new = b"auto rtCfg  = MakeValidRuntimeConfiguration();"

count = data.count(old)

if count:
    path.write_bytes(data.replace(old, new))
    print(f"Updated {count} occurrence(s).")
else:
    print("No exact matches found.")


