
from pathlib import Path

needle = "auto rtCfg = MakeValidRuntimeConfiguration"
hidden_chars = "\u00a0\u200b\u200c\u200d\u2060\ufeff"

path = Path(
    "/Users/mthiru270@apac.comcast.com/repos/stability/"
    "entservices-appmanagers/Tests/L0Tests/RuntimeManager/"
    "RuntimeManager_ComponentTests.cpp"
)

original = path.read_bytes()
text = original.decode("utf-8")

lines = text.splitlines(keepends=True)
changed = False

for i, line in enumerate(lines):
    # Process only matching lines
    if needle not in line:
        continue

    if line.endswith("\r\n"):
        ending = "\r\n"
        body = line[:-2]
    elif line.endswith(("\n", "\r")):
        ending = line[-1]
        body = line[:-1]
    else:
        ending = ""
        body = line

    cleaned = body.translate(
        {ord(ch): None for ch in hidden_chars}
    ).rstrip(" \t")

    if cleaned != body:
        lines[i] = cleaned + ending
        changed = True
        print(f"Fixed: {path}:{i + 1}")

if changed:
    path.write_bytes("".join(lines).encode("utf-8"))
    print("Changes saved.")
else:
    print("No matching lines with these hidden characters or trailing spaces found.")

