#!/usr/bin/env python3
import argparse
import itertools
import json
import random
from pathlib import Path
from typing import Dict, List

RANDOM = random.Random(42)

INT_BOUNDARIES = {
    "int8": [-128, -1, 0, 1, 127],
    "uint8": [0, 1, 255],
    "int16": [-32768, -1, 0, 1, 32767],
    "uint16": [0, 1, 65535],
    "int32": [-2147483648, -1, 0, 1, 2147483647],
    "uint32": [0, 1, 4294967295],
    "int64": [-9223372036854775808, -1, 0, 1, 9223372036854775807],
    "uint64": [0, 1, 18446744073709551615],
}


def type_key(arg_type: str) -> str:
    t = arg_type.lower().replace(" ", "")
    if "uint64" in t or "unsignedlonglong" in t:
        return "uint64"
    if "int64" in t or "longlong" in t:
        return "int64"
    if "uint32" in t or "unsignedint" in t:
        return "uint32"
    if "int32" in t or "int" == t:
        return "int32"
    if "uint16" in t or "unsignedshort" in t:
        return "uint16"
    if "int16" in t or "short" in t:
        return "int16"
    if "uint8" in t or "unsignedchar" in t:
        return "uint8"
    if "int8" in t or "char" == t:
        return "int8"
    if "bool" in t:
        return "bool"
    if "string" in t or "char*" in t or "constchar*" in t:
        return "string"
    if "json" in t:
        return "json"
    if "*" in t:
        return "pointer"
    return "generic"


def value_pool(arg_type: str) -> List[object]:
    key = type_key(arg_type)
    if key in INT_BOUNDARIES:
        return INT_BOUNDARIES[key] + [255, -1, 99999999]
    if "bool" == key:
        return [True, False, 0, 1, -1, 2]
    if "string" == key:
        return [
            "",
            " ",
            "normal",
            "A" * 4096,
            "!@#$%^&*()_+-=[]{};':,./<>?",
            "line1\nline2\tend",
            "00:00:00:00:00:00",
            "invalid-mac-zz",
            "eth999999",
        ]
    if "pointer" == key:
        return [None, {}, {"partial": True}, {"nested": {"bad": 1}}]
    if "json" == key:
        return [{"k": "v"}, {"k": None}, {"k": 1}, {"k": []}]
    return [0, 1, "x", None]


def generate_json_seeds(max_count: int) -> List[Dict[str, object]]:
    seeds = [
        {"name": "valid", "payload": {"id": "app", "state": "active", "enabled": True}},
        {"name": "missing_keys", "payload": {"id": "app"}},
        {"name": "empty_values", "payload": {"id": "", "state": ""}},
        {"name": "null_values", "payload": {"id": None, "state": None}},
        {"name": "invalid_types", "payload": {"id": 123, "state": ["x"]}},
        {"name": "nested_object", "payload": {"outer": {"inner": {"deep": "x"}}}},
        {"name": "array_mutation", "payload": {"items": [{"id": 1}, None, "bad"]}},
        {"name": "special_chars", "payload": {"text": "\n\t\u0000"}},
    ]

    base = {"required": "x", "optional": "y", "count": 1, "nested": {"k": "v"}}
    keys = list(base.keys())
    for k in keys:
        m = dict(base)
        m.pop(k, None)
        seeds.append({"name": f"remove_{k}", "payload": m})

        for variant_name, variant in [
            ("empty", ""),
            ("whitespace", "   "),
            ("null", None),
            ("oversized", "Z" * 5000),
            ("invalid_type", [1, 2, 3]),
        ]:
            c = json.loads(json.dumps(base))
            c[k] = variant
            seeds.append({"name": f"{k}_{variant_name}", "payload": c})

    while len(seeds) < max_count:
        seeds.append({"name": f"random_{len(seeds)}", "payload": {"value": RANDOM.randint(-1000, 1000)}})

    return seeds[:max_count]


def write_target_seeds(target: Dict[str, object], out_dir: Path, pairwise_budget: int, state_budget: int, json_cap: int) -> int:
    key = target["stable_target_key"]
    tdir = out_dir / key
    tdir.mkdir(parents=True, exist_ok=True)

    args = target.get("arguments", [])
    pools = [value_pool(a.get("type", "")) for a in args]

    written = 0
    for i, arg in enumerate(args):
        vals = pools[i][: min(16, len(pools[i]))]
        for v in vals:
            p = {"target": key, "mode": "single-arg", "arg_index": i, "arg": arg, "value": v}
            (tdir / f"seed_arg{i}_{written:03d}.json").write_text(json.dumps(p, indent=2) + "\n", encoding="utf-8")
            written += 1

    if len(args) > 1:
        combos = []
        indices = list(range(len(args)))
        for i, j in itertools.combinations(indices, 2):
            left = pools[i][: min(8, len(pools[i]))]
            right = pools[j][: min(8, len(pools[j]))]
            for lv, rv in itertools.product(left, right):
                combos.append((i, j, lv, rv))
        for idx, (i, j, lv, rv) in enumerate(combos[:pairwise_budget]):
            p = {"target": key, "mode": "pairwise", "left_index": i, "right_index": j, "left": lv, "right": rv}
            (tdir / f"seed_pair_{idx:03d}.json").write_text(json.dumps(p, indent=2) + "\n", encoding="utf-8")
            written += 1

    tags = [x.lower() for x in target.get("classification_tags", [])]
    if any(t in tags for t in ["scheduler", "event", "api", "callable"]):
        for i in range(state_budget):
            p = {
                "target": key,
                "mode": "state-sequence",
                "sequence": ["init", "start", f"mutate_{i}", "stop"],
            }
            (tdir / f"seed_state_{i:03d}.json").write_text(json.dumps(p, indent=2) + "\n", encoding="utf-8")
            written += 1

    if target.get("json_touching", False):
        json_seeds = generate_json_seeds(max(30, min(40, json_cap)))
        for idx, seed in enumerate(json_seeds[:json_cap]):
            (tdir / f"seed_json_{idx:03d}.json").write_text(json.dumps(seed, indent=2) + "\n", encoding="utf-8")
            written += 1

    # Pointer-mode seeds are consumed by generated harness InputReader.
    # 0 -> nullptr, 1 -> invalid pointer sentinel, 2 -> valid stack pointer.
    for mode in (0, 1, 2):
        p = {
            "target": key,
            "mode": "pointer-mode",
            "pointer_mode": mode,
            "note": "0=nullptr,1=invalid,2=valid",
        }
        (tdir / f"seed_ptrmode_{mode}.json").write_text(json.dumps(p, indent=2) + "\n", encoding="utf-8")
        written += 1

    return written


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate semantic fuzz corpus")
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--manifest", default="fuzz/stats/results/discovery_manifest.json")
    parser.add_argument("--out-dir", default="fuzz/stats/corpus")
    parser.add_argument("--summary-out", default="fuzz/stats/results/corpus_summary.json")
    parser.add_argument("--pairwise-budget", type=int, default=48)
    parser.add_argument("--state-sequence-seeds", type=int, default=20)
    parser.add_argument("--max-json-seeds", type=int, default=40)
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    out_dir = (repo_root / args.out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    manifest = json.loads((repo_root / args.manifest).read_text(encoding="utf-8"))
    targets = manifest.get("targets", [])

    total = 0
    target_counts = {}
    for target in targets:
        count = write_target_seeds(
            target,
            out_dir,
            args.pairwise_budget,
            args.state_sequence_seeds,
            args.max_json_seeds,
        )
        target_counts[target["stable_target_key"]] = count
        total += count

    summary = {
        "total_targets": len(targets),
        "total_seeds": total,
        "target_seed_counts": target_counts,
        "pairwise_budget": args.pairwise_budget,
        "state_sequence_seeds": args.state_sequence_seeds,
        "max_json_seeds": args.max_json_seeds,
    }
    summary_path = (repo_root / args.summary_out).resolve()
    summary_path.parent.mkdir(parents=True, exist_ok=True)
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
