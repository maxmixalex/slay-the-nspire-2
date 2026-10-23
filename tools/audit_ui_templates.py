#!/usr/bin/env python3
"""Audit every locale UI binding without exporting private localized prose.

Read one or more private NSP2/17, NSP2/18 or NSP2/19 packs. The optional native runner
checks actual display routes and emits only slot IDs, field names and counts.
This inventory does not replace Content::open's integrity/structural checks.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import mmap
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from locale_profile import ROOT, UI_BASE, bindings_and_keys, validate_generated

FIELD = re.compile(r"\{([A-Za-z_][A-Za-z_0-9.]*)")
STATUS_ROW = re.compile(r'\{\s*("(?:\\.|[^"\\])*")\s*,\s*("(?:\\.|[^"\\])*")\s*,')
UI_FIELDS = {"Amount", "Hotkey", "HotKey", "ascension", "actNumber", "singleStarIcon"}


def status_sources():
    descriptions = {}
    count = 0
    for name in ("status.hpp", "extended_status_definitions.inc",
                 "character_status_definitions.inc", "five_character_status_definitions.inc"):
        for match in STATUS_ROW.finditer((ROOT / "include" / name).read_text()):
            source = json.loads(match[2])
            descriptions.setdefault(source, []).append(count)
            count += 1
    keys = json.loads((ROOT / "config/locale-status-keys.json").read_text())["keys"]
    if count != len(keys):
        raise ValueError(f"Status metadata order/count drift: {count} != {len(keys)}")
    return descriptions


def display_route(row, fields, statuses):
    if not fields:
        return "static", []
    if row["source"] in statuses:
        return "status", statuses[row["source"]]
    reference = row.get("reference", "")
    if reference.startswith("relics."):
        return "relic", []
    if reference.startswith("card_selection.") and set(fields) <= {"Amount"}:
        return "selection", []
    if reference in ("static_hover_tips.DECK.title", "static_hover_tips.EXHAUST_PILE.title"):
        return "hotkey", []
    if reference == "gameplay_ui.ASCENSION_LEVEL":
        return "ascension", []
    if set(fields) == {"actNumber"}:
        return "act", []
    return "unclassified", []


def pack_inventory(path, rows, statuses):
    with path.open("rb") as handle, mmap.mmap(handle.fileno(), 0, access=mmap.ACCESS_READ) as data:
        if len(data) < 40:
            raise ValueError(f"Short pack: {path.name}")
        magic, version, reference, length, images, glyphs, strings, clips, cards, _ = struct.unpack_from("<10I", data)
        if (magic != 0x3250534E or version not in (17, 18, 19) or reference != 0x41CEF1EA
                or length != len(data) or strings < UI_BASE + len(rows)):
            raise ValueError(f"Unsupported/truncated locale pack: {path.name}")
        directory = 40 + 16 * (images + glyphs)
        if directory + strings * 8 > len(data):
            raise ValueError(f"Out-of-bounds string directory: {path.name}")

        def string(index):
            offset, count = struct.unpack_from("<II", data, directory + index * 8)
            if offset > len(data) or count >= len(data) - offset or data[offset + count] != 0:
                raise ValueError(f"Out-of-bounds/unterminated string slot {index}")
            return data[offset:offset + count].decode("utf-8")

        language = string(6110)
        if language not in ("eng", "zhs"):
            raise ValueError(f"Unknown language marker: {language!r}")
        all_bindings, dynamic = [], []
        counts, routes = Counter(), Counter()
        table_digest = hashlib.sha256()
        for slot, row in enumerate(rows, UI_BASE):
            raw = string(slot)
            encoded = raw.encode("utf-8")
            table_digest.update(struct.pack("<II", slot, len(encoded)))
            table_digest.update(encoded)
            fields = sorted(set(FIELD.findall(raw)))
            route, status_ids = display_route(row, fields, statuses)
            entry = {"slot": slot, "reference": row.get("reference"), "route": route,
                     "status_ids": status_ids, "fields": fields,
                     "utf8_bytes": len(encoded), "sha256": hashlib.sha256(encoded).hexdigest()}
            # This private-in-memory value is written only to the runner's
            # temporary catalog. It is a public source literal, never raw text.
            all_bindings.append((entry, row["source"]))
            if fields:
                dynamic.append(entry)
                counts.update(fields)
                routes[route] += 1
        digest = hashlib.sha256()
        for offset in range(0, len(data), 1024 * 1024):
            digest.update(data[offset:offset + 1024 * 1024])
        report = {
            "file": path.name, "bytes": len(data), "sha256": digest.hexdigest(),
            "version": version, "language": language, "reference": "v0.111.0 / 41cef1ea",
            "bindings": len(rows), "dynamic_bindings": len(dynamic),
            "ui_table_sha256": table_digest.hexdigest(),
            "field_binding_counts": dict(sorted(counts.items())),
            "route_binding_counts": dict(sorted(routes.items())),
            "unclassified_slots": [row["slot"] for row in dynamic if row["route"] == "unclassified"],
            "unsupported_ui_fields": [
                {"slot": row["slot"], "fields": sorted(set(row["fields"]) - UI_FIELDS)}
                for row in dynamic if row["route"] in ("selection", "hotkey", "ascension", "act")
                and set(row["fields"]) - UI_FIELDS
            ],
            "dynamic": dynamic,
        }
        return report, all_bindings


def run_routes(runner, pack, bindings):
    # Hex encoding makes the manifest unambiguous for source literals with
    # newlines/tabs. Nothing in this file is original-pack localized prose.
    with tempfile.TemporaryDirectory(prefix="spire-ui-audit-") as temporary:
        catalog = Path(temporary) / "bindings.tsv"
        catalog.write_text("".join(
            f"{row['slot']}\t{row['route']}\t{','.join(map(str, row['status_ids'])) or '-'}\t"
            f"{row['reference'] or '-'}\t{source.encode('utf-8').hex()}\n"
            for row, source in bindings
        ))
        result = subprocess.run([str(runner), str(pack), str(catalog)], text=True, capture_output=True)
        observations = []
        for line in result.stdout.splitlines():
            if line.strip():
                observations.append(json.loads(line))
        # The runner's diagnostics name assertions/slots only. Keep unexpected
        # native failures visible; never silently convert a failed run to pass.
        if result.stderr:
            print(result.stderr, end="", file=__import__("sys").stderr)
        return {"exit_code": result.returncode, "observations": observations}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pack", action="append", required=True, metavar="LABEL=PATH")
    parser.add_argument("--runner", type=Path, help="Built ui_text_regression native executable")
    parser.add_argument("--output", type=Path, help="Write complete metadata-only JSON inventory")
    args = parser.parse_args()
    validate_generated()
    rows, _ = bindings_and_keys()
    statuses = status_sources()
    report = {"schema": 1, "scope": "UI binding inventory and actual display-route regression; no private prose", "packs": {}}
    failed = False
    for argument in args.pack:
        label, separator, filename = argument.partition("=")
        if not separator or not label or label in report["packs"]:
            parser.error("Each --pack must have a unique LABEL=PATH")
        path = Path(filename).resolve()
        result, bindings = pack_inventory(path, rows, statuses)
        failed |= bool(result["unclassified_slots"] or result["unsupported_ui_fields"])
        if args.runner:
            result["runtime"] = run_routes(args.runner.resolve(), path, bindings)
            failed |= result["runtime"]["exit_code"] != 0
        report["packs"][label] = result
    report["passed"] = not failed
    encoded = json.dumps(report, ensure_ascii=True, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded)
        print(json.dumps({"passed": not failed, "output": str(args.output), "packs": {
            name: {key: item[key] for key in ("version", "language", "bindings", "dynamic_bindings", "route_binding_counts")}
            for name, item in report["packs"].items()
        }}))
    else:
        print(encoded, end="")
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
