#!/usr/bin/env python3
"""Fix all four spec Covered Code sections and add missing libpackage-sky sections."""
import os
import re

ROOT = '/Users/mthiru270@apac.comcast.com/repos/capabilities'
CODE_EXT = {'.h', '.hpp', '.c', '.cc', '.cpp'}
SKIP_DIRS = {'Tests', 'test', 'tests', '__pycache__', '.git', 'build', 'cmake', 'CMakeFiles', 'generated'}
KEYWORDS = {
    'if', 'for', 'while', 'switch', 'return', 'sizeof', 'catch', 'do', 'else', 'try',
    'new', 'delete', 'namespace', 'assert', 'ASSERT', 'LOGINFO', 'LOGERR', 'LOGWARN',
    'LOGDBG', 'LOGTRACE', 'LOGENTRY', 'BEGIN_INTERFACE_MAP', 'INTERFACE_ENTRY',
    'END_INTERFACE_MAP', 'SERVICE_REGISTRATION', 'VARIABLE_IS_NOT_USED', 'DEPRECATED',
    'ENUM_CONVERSION_HANDLER', 'ENUM_CONVERSION_END', 'main', 'IMPLEMENT_MODIFIER',
}

PAT_SCOPED = re.compile(r'\b([A-Za-z_~][A-Za-z0-9_:<>~]*)::([A-Za-z_~][A-Za-z0-9_]*)\s*\(')
PAT_FUNC = re.compile(
    r'^\s*(?:(?:inline|static|virtual|explicit|friend|constexpr|extern)\s+)*'
    r'(?:[A-Za-z_:~][A-Za-z0-9_:<>*&\s,]+\s+)?([A-Za-z_~][A-Za-z0-9_]*)\s*\('
    r'[^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?(?:=\s*0\s*)?[;{]'
)


def get_file_methods(path):
    methods = []
    seen = set()
    try:
        for line in open(path, encoding='utf-8', errors='ignore'):
            m = PAT_SCOPED.search(line)
            if m:
                cls, fn = m.group(1), m.group(2)
                if fn not in KEYWORDS and cls not in KEYWORDS:
                    name = f"{cls}::{fn}"
                    if name not in seen:
                        seen.add(name)
                        methods.append(name)
                continue
            m = PAT_FUNC.match(line)
            if m:
                name = m.group(1)
                if name not in KEYWORDS and name not in seen:
                    seen.add(name)
                    methods.append(name)
    except Exception:
        pass
    return methods[:12]


def scan_component(comp):
    files = {}
    comp_root = os.path.join(ROOT, comp)
    for dp, dirs, fnames in os.walk(comp_root):
        dirs[:] = sorted(d for d in dirs if d not in SKIP_DIRS)
        for f in sorted(fnames):
            if os.path.splitext(f)[1] not in CODE_EXT:
                continue
            path = os.path.join(dp, f)
            rel = os.path.relpath(path, ROOT)
            files[rel] = get_file_methods(path)
    return files


def build_covered_code_block(files_map):
    lines = ['## Covered Code', '']
    for fpath, methods in sorted(files_map.items()):
        lines.append(f'- {fpath}:')
        if methods:
            for m in methods:
                lines.append(f'    - {m}')
        else:
            lines.append('    - _(file-level coverage)_')
    return '\n'.join(lines)


def replace_covered_code_section(spec_path, new_block):
    content = open(spec_path, encoding='utf-8').read()
    start_m = re.search(r'^## Covered Code\s*\n', content, re.MULTILINE)
    if not start_m:
        oq = re.search(r'^## Open Queries', content, re.MULTILINE)
        if oq:
            content = content[:oq.start()] + new_block + '\n\n' + content[oq.start():]
        else:
            content = content.rstrip() + '\n\n' + new_block + '\n'
    else:
        rest_start = start_m.end()
        end_m = re.search(r'^## [^\n]', content[rest_start:], re.MULTILINE)
        end_pos = rest_start + end_m.start() if end_m else len(content)
        content = content[:start_m.start()] + new_block + '\n\n' + content[end_pos:]
    open(spec_path, 'w', encoding='utf-8').write(content)


# -----------------------------------------------
# 1. entservices-apis
# -----------------------------------------------
files = scan_component('entservices-apis')
block = build_covered_code_block(files)
replace_covered_code_section(os.path.join(ROOT, 'openspec/specs/entservices-apis/spec.md'), block)
print(f"entservices-apis: {len(files)} files mapped")

# -----------------------------------------------
# 2. rdk-window-manager
# -----------------------------------------------
files = scan_component('rdk-window-manager')
block = build_covered_code_block(files)
replace_covered_code_section(os.path.join(ROOT, 'openspec/specs/rdk-window-manager/spec.md'), block)
print(f"rdk-window-manager: {len(files)} files mapped")

# -----------------------------------------------
# 3. libpackage-sky
# -----------------------------------------------
files = scan_component('libpackage-sky')
block = build_covered_code_block(files)
replace_covered_code_section(os.path.join(ROOT, 'openspec/specs/libpackage-sky/spec.md'), block)
print(f"libpackage-sky: {len(files)} files mapped")

# -----------------------------------------------
# 4. entservices-appmanagers — append remaining unmapped files
# -----------------------------------------------
sp = os.path.join(ROOT, 'openspec/specs/entservices-appmanagers/spec.md')
already_mapped = set()
in_cov = False
for line in open(sp, encoding='utf-8'):
    if '## Covered Code' in line:
        in_cov = True
        continue
    if line.startswith('## ') and in_cov:
        break
    if in_cov:
        m = re.match(r'-\s+([^:]+):\s*$', line.rstrip())
        if m:
            already_mapped.add(m.group(1).strip())

all_files = scan_component('entservices-appmanagers')
remaining = {k: v for k, v in all_files.items() if k not in already_mapped}

content = open(sp, encoding='utf-8').read()
start_m = re.search(r'^## Covered Code\s*\n', content, re.MULTILINE)
rest_start = start_m.end()
end_m = re.search(r'^## Open Queries', content[rest_start:], re.MULTILINE)
if end_m:
    insert_pos = rest_start + end_m.start()
else:
    insert_pos = len(content)

extra_lines = []
for fpath, methods in sorted(remaining.items()):
    extra_lines.append(f'- {fpath}:')
    if methods:
        for m in methods:
            extra_lines.append(f'    - {m}')
    else:
        extra_lines.append('    - _(file-level coverage)_')

if extra_lines:
    content = content[:insert_pos] + '\n'.join(extra_lines) + '\n\n' + content[insert_pos:]
    open(sp, 'w', encoding='utf-8').write(content)

print(f"entservices-appmanagers: {len(remaining)} additional files added (total {len(all_files)})")
print("Done.")
