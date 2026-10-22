
from pathlib import Path
import re

path = Path(
    "Tests/L0Tests/RuntimeManager/"
    "RuntimeManager_ComponentTests.cpp"
)

data = path.read_bytes()

print("File:", path)
print("CRLF count:", data.count(b"\r\n"))
print("LF count:", data.count(b"\n"))
print("Standalone CR count:", data.count(b"\r") - data.count(b"\r\n"))

text = data.decode("utf-8")

for number, line in enumerate(text.splitlines(keepends=True), 1):
    if "MakeValidRuntimeConfiguration" in line:
        print(f"\nLine {number}: {line!r}")
        print("Bytes:", line.encode("utf-8").hex(" "))
        print(
            "Whitespace code points:",
            [
                f"U+{ord(ch):04X}"
                for ch in line
                if ch.isspace() and ch not in " \t\r\n"
            ],
        )

        if re.search(r"rtCfg\s*=\s*MakeValidRuntimeConfiguration", line):
            print("Confirmed: matching assignment")

