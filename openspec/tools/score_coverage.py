#!/usr/bin/env python3
"""Compute OpenSpec coverage score across all 7 dimensions."""
import os
import re

ROOT = '/Users/mthiru270@apac.comcast.com/repos/capabilities'
CODE_EXT = {'.h', '.hpp', '.c', '.cc', '.cpp'}
SKIP_DIRS = {'Tests', 'test', 'tests', '__pycache__', '.git', 'build', 'cmake', 'CMakeFiles'}
PAT_SCOPED = re.compile(r'\b([A-Za-z_~][A-Za-z0-9_:<>~]*)::([A-Za-z_~][A-Za-z0-9_]*)\s*\(')
KEYWORDS = {'if', 'for', 'while', 'switch', 'return', 'sizeof', 'catch', 'do', 'else', 'try',
            'new', 'delete', 'namespace', 'assert', 'ASSERT', 'LOGINFO', 'LOGERR', 'LOGWARN', 'LOGDBG'}

COMPS = ['entservices-appmanagers', 'entservices-apis', 'libpackage-sky', 'rdk-window-manager']
SPECS_DIR = os.path.join(ROOT, 'openspec/specs')
SPEC_FILES = [os.path.join(SPECS_DIR, c, 'spec.md') for c in COMPS]


def get_methods(path):
    ms = []
    seen = set()
    for line in open(path, encoding='utf-8', errors='ignore'):
        m = PAT_SCOPED.search(line)
        if m and m.group(2) not in KEYWORDS:
            n = f"{m.group(1)}::{m.group(2)}"
            if n not in seen:
                seen.add(n)
                ms.append(n)
    return ms


total_m = 0
covered_m = 0

for comp in COMPS:
    sp = os.path.join(SPECS_DIR, comp, 'spec.md')
    code_methods = {}
    for dp, dirs, files in os.walk(os.path.join(ROOT, comp)):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for f in sorted(files):
            if os.path.splitext(f)[1] not in CODE_EXT:
                continue
            path = os.path.join(dp, f)
            rel = os.path.relpath(path, ROOT)
            code_methods[rel] = get_methods(path)

    mapped_files = set()
    if os.path.exists(sp):
        in_cov = False
        for line in open(sp, encoding='utf-8'):
            if line.strip() == '## Covered Code':
                in_cov = True
                continue
            if line.startswith('## ') and in_cov:
                break
            if in_cov:
                m = re.match(r'-\s+([^:]+):\s*$', line.rstrip())
                if m:
                    mapped_files.add(m.group(1).strip())

    cm = sum(len(code_methods.get(f, [])) for f in mapped_files)
    tm = sum(len(v) for v in code_methods.values())
    total_m += tm
    covered_m += cm

# === CODE TO SPEC COVERAGE (40) ===
ref_cov = 100 * covered_m / total_m if total_m else 0
ref_score = min(20, ref_cov / 100 * 20)
exist_score = 10 * sum(1 for f in SPEC_FILES if os.path.exists(f)) / len(SPEC_FILES)
required_sections = ['overview', 'description', 'requirements', 'covered code']
completeness = []
for sp in SPEC_FILES:
    if not os.path.exists(sp):
        completeness.append(0)
        continue
    c = open(sp, encoding='utf-8').read().lower()
    completeness.append(sum(1 for s in required_sections if s in c) / len(required_sections))
comp_score = 5 * sum(completeness) / len(completeness)
orphan_score = min(5, ref_cov / 100 * 5)
code_total = ref_score + exist_score + comp_score + orphan_score

# === ARCHITECTURE HLA (10) ===
arch = os.path.join(SPECS_DIR, 'system-integration', 'architecture-narrative.md')
arch_c = open(arch, encoding='utf-8').read() if os.path.exists(arch) else ''
arch_score = (
    3
    + 3 * (1 if '```' in arch_c else 0)
    + 2
    + 2 * (1 if 'Traceability' in arch_c or ('.cpp' in arch_c and 'Implementation' in arch_c) else 0)
)

# === PERFORMANCE (10) ===
perf_scores = []
for sp in SPEC_FILES:
    if not os.path.exists(sp):
        perf_scores.append(0)
        continue
    c = open(sp, encoding='utf-8').read()
    s = (
        3
        + 3 * (1 if re.search(r'\|\s*[^|]+\|\s*[^|]+\|\s*[^|]+\|', c) else 0)
        + 2 * (1 if re.search(r'Pass.*Total|pass.*total|PASS', c) else 0)
        + 2 * (1 if re.search(r'Measured|PASS|benchmark|Benchmark', c) else 0)
    )
    perf_scores.append(min(10, s))
perf_score = sum(perf_scores) / len(perf_scores)

# === EXTERNAL INTERFACE (10) ===
ext_scores = []
for sp in SPEC_FILES:
    if not os.path.exists(sp):
        ext_scores.append(0)
        continue
    c = open(sp, encoding='utf-8').read()
    s = (
        3
        + 3 * (1 if re.search(r'hresult|virtual|std::', c) else 0)
        + 2 * (1 if re.search(r'Usage Example|usage example|Example Usage', c) else 0)
        + 2 * (1 if re.search(r'Interface Validation|Validation Test', c) else 0)
    )
    ext_scores.append(min(10, s))
ext_score = sum(ext_scores) / len(ext_scores)

# === SECURITY (10) ===
sec_scores = []
for sp in SPEC_FILES:
    if not os.path.exists(sp):
        sec_scores.append(0)
        continue
    c = open(sp, encoding='utf-8').read()
    s = (
        3
        + 3 * (1 if re.search(r'[Tt]hreat\s+[Mm]odel|[Tt]hreat\s+[Aa]nalysis', c) else 0)
        + 2 * (1 if re.search(r'[Ss]ecurity [Rr]equirement|SR-', c) else 0)
        + 2 * (1 if re.search(r'Security Validation|PASS', c) else 0)
    )
    sec_scores.append(min(10, s))
sec_score = sum(sec_scores) / len(sec_scores)

# === VERSIONING (10) ===
ver_scores = []
for sp in SPEC_FILES:
    if not os.path.exists(sp):
        ver_scores.append(0)
        continue
    c = open(sp, encoding='utf-8').read()
    s = (
        3
        + 3 * (1 if re.search(r'[Ss]emantic [Vv]ersion|MAJOR.*MINOR', c) else 0)
        + 2 * (1 if re.search(r'[Bb]ackward [Cc]ompatib|[Ff]orward [Cc]ompatib', c) else 0)
        + 2 * (1 if re.search(r'[Mm]igration.*[Pp]ath|[Uu]pgrade [Pp]ath', c) else 0)
    )
    ver_scores.append(min(10, s))
ver_score = sum(ver_scores) / len(ver_scores)

# === CONFORMANCE (10) ===
conf_scores = []
for sp in SPEC_FILES:
    if not os.path.exists(sp):
        conf_scores.append(0)
        continue
    c = open(sp, encoding='utf-8').read()
    s = (
        3
        + 3 * (1 if re.search(r'[Pp]ass\s*/\s*[Tt]otal|\d+\s*/\s*\d+\s*\|', c) else 0)
        + 2 * (1 if re.search(r'[Hh]ow to [Rr]un|cmake|ctest', c) else 0)
        + 2 * (1 if re.search(r'Validation Results|PASS.*PASS|GoogleTest', c) else 0)
    )
    conf_scores.append(min(10, s))
conf_score = sum(conf_scores) / len(conf_scores)

TOTAL = code_total + arch_score + perf_score + ext_score + sec_score + ver_score + conf_score

print("=" * 60)
print(f"  OpenSpec Coverage Score: {TOTAL:.1f} / 100")
print("=" * 60)
print(f"  Code to Spec Coverage:      {code_total:.1f} / 40")
print(f"    Reference Coverage:       {ref_score:.1f} / 20  ({ref_cov:.1f}%)")
print(f"    Spec Existence:           {exist_score:.1f} / 10")
print(f"    Spec Completeness:        {comp_score:.1f} / 5")
print(f"    No Orphaned Code:         {orphan_score:.1f} / 5")
print(f"  Architecture HLA:           {arch_score:.1f} / 10")
print(f"  Performance:                {perf_score:.1f} / 10")
print(f"  External Interface:         {ext_score:.1f} / 10")
print(f"  Security:                   {sec_score:.1f} / 10")
print(f"  Versioning & Compatibility: {ver_score:.1f} / 10")
print(f"  Conformance Testing:        {conf_score:.1f} / 10")
