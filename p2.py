
from pathlib import Path
import re

path = Path(
    "Tests/L0Tests/RuntimeManager/"
    "RuntimeManager_ComponentTests.cpp"
)

data = path.read_bytes()
text = data.decode("utf-8")

# Normalize only matching assignment lines.
pattern = re.compile(
    r"(?m)^([ \t]*)auto[ \t\u00a0]*rtCfg"
    r"[ \t\u00a0]*=[ \t\u00a0]*"
    r"MakeValidRuntimeConfiguration\(\);"
)

updated, count = pattern.subn(
    r"\1auto rtCfg = MakeValidRuntimeConfiguration();",
    text
)

if updated != text:
    path.write_bytes(updated.encode("utf-8"))
    print(f"Fixed {count} matching lines.")
else:
    print("No changes needed.")


