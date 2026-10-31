#!/usr/bin/env python3
"""Build the two matching calculator editions and one optional-language ZIP.

All private resources remain outside the source repository. Installation sizes
are the launcher plus every required .tns file, in decimal bytes, before saves.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import zipfile

from split_content import LANGUAGES, reconstruct
from storage_parts import PART_FILES, join_parts

ROOT = Path(__file__).resolve().parents[1]
PROFILES = ("animated", "no-animations")


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def inspect_resources(directory):
    common = (directory / "nSpire2.dat.tns").read_bytes()
    required = ["nSpire2.dat.tns", "nSpire2.eng.dat.tns", "nSpire2.zhs.dat.tns"]
    if common[:4] == b"NSM1":
        common = join_parts(common, {name: (directory / name).read_bytes() for name in PART_FILES})
        required.extend(PART_FILES)
    locales = {}
    for code in LANGUAGES:
        path = directory / ("nSpire2." + code + ".dat.tns")
        logical = reconstruct(common, path.read_bytes(), code)
        locales[code] = {"bytes": path.stat().st_size, "sha256": digest(path),
                         "logical_bytes": len(logical),
                         "logical_sha256": hashlib.sha256(logical).hexdigest()}
    return {"required_bytes": sum((directory / name).stat().st_size for name in required),
            "required_files": required, "locales": locales}


def check_archive(path):
    with zipfile.ZipFile(path) as archive:
        failure = archive.testzip()
        if failure is not None:
            raise ValueError("ZIP CRC failed: " + failure)
        files = archive.infolist()
        if len({item.filename for item in files}) != len(files):
            raise ValueError("Duplicate ZIP member in " + path.name)
        return {"file": path.name, "zip_bytes": path.stat().st_size,
                "extracted_bytes": sum(item.file_size for item in files),
                "sha256": digest(path), "members": len(files)}


def build(args):
    args.output.mkdir(parents=True, exist_ok=True)
    launcher_bytes = args.launcher.stat().st_size
    directories = {"animated": args.animated, "no-animations": args.no_animations}
    resources = {name: inspect_resources(path) for name, path in directories.items()}
    for row in resources.values():
        row["installed_bytes"] = launcher_bytes + row["required_bytes"]
    meets_budget = resources["animated"]["installed_bytes"] <= args.maximum_animated_bytes
    if not meets_budget and not args.allow_over_budget_candidate:
        raise ValueError("Animated installation is %d bytes, above the %d-byte limit" %
                         (resources["animated"]["installed_bytes"], args.maximum_animated_bytes))
    base_version = (ROOT / "VERSION").read_text().strip().split("-", 1)[0]
    stem = "Slay-the-Nspire-2-" + base_version + "-ui23"
    receipts = {}
    with tempfile.TemporaryDirectory(prefix="nspire-package-variants-") as temporary:
        stage = Path(temporary)
        for profile, directory in directories.items():
            destination = stage / profile
            command = [sys.executable, str(ROOT / "tools" / "package_release.py"),
                       "--launcher", str(args.launcher), "--common", str(directory / "nSpire2.dat.tns"),
                       "--eng", str(directory / "nSpire2.eng.dat.tns"),
                       "--zhs", str(directory / "nSpire2.zhs.dat.tns"),
                       "--output", str(destination), "--calculator-only",
                       "--runtime-checkpoint", args.runtime_checkpoint,
                       "--resource-checkpoint", args.resource_checkpoint,
                       "--stage", "ui23-" + profile]
            subprocess.run(command, check=True)
            candidates = list(destination.glob("*-calculator.zip"))
            if len(candidates) != 1:
                raise ValueError("Expected exactly one calculator archive")
            archive = args.output / (stem + "-" + profile + "-bilingual-calculator.zip")
            candidates[0].replace(archive)
            receipts[profile] = check_archive(archive)
            receipts[profile]["installed_bytes"] = resources[profile]["installed_bytes"]

    extras = [code for code in LANGUAGES if code not in ("eng", "zhs")]
    optional = args.output / (stem + "-optional-languages.zip")
    instructions = (
        "Slay the Nspire 2 UI23 — optional original languages\n\n"
        "请选择与你安装的版本对应的文件夹：\n"
        "animated = 完整动画版；no-animations = 无人物/怪物连续动画版。\n"
        "仅将其中所需的 data/nSpire2.<language>.dat.tns 复制到游戏 data 文件夹。\n"
        "两个版本的语言寻址不同，请勿混用。英语和简体中文已包含在两个完整安装包。\n\n"
        "Choose the directory matching your calculator edition: animated or no-animations.\n"
        "Copy only the desired data/nSpire2.<language>.dat.tns files into the game's data folder.\n"
        "The editions use different resource addresses. Do not mix their locale files.\n"
        "English and Simplified Chinese are already in both complete installers.\n\n"
        "Available additional locale codes: " + ", ".join(extras) + "\n")
    with zipfile.ZipFile(optional, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        archive.writestr("README-languages.txt", instructions.encode("utf-8"))
        for profile, directory in directories.items():
            for code in extras:
                name = "nSpire2." + code + ".dat.tns"
                archive.write(directory / name, profile + "/data/" + name)
    with zipfile.ZipFile(optional) as archive:
        for profile, directory in directories.items():
            for code in extras:
                name = "nSpire2." + code + ".dat.tns"
                if archive.read(profile + "/data/" + name) != (directory / name).read_bytes():
                    raise ValueError("Optional locale readback differs")
    receipts["optional-languages"] = check_archive(optional)
    result = {"schema": 1, "stage": "ui23", "runtime_checkpoint": args.runtime_checkpoint,
              "resource_checkpoint": args.resource_checkpoint,
              "maximum_animated_bytes": args.maximum_animated_bytes,
              "animated_storage_target_met": meets_budget,
              "physical_device_tested": False,
              "physical_device_note": "Packaging verifies files and does not establish calculator FPS or physical peak RAM.",
              "launcher_bytes": launcher_bytes, "launcher_sha256": digest(args.launcher),
              "resources": resources, "archives": receipts}
    manifest = args.output / "UI23-delivery-manifest.json"
    manifest.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("launcher", "animated", "no-animations", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--runtime-checkpoint", required=True)
    parser.add_argument("--resource-checkpoint", required=True)
    parser.add_argument("--maximum-animated-bytes", type=int, default=70_000_000)
    parser.add_argument("--allow-over-budget-candidate", action="store_true",
                        help="Produce explicitly over-budget evaluation artifacts; never marks the target met")
    args = parser.parse_args()
    if args.maximum_animated_bytes <= 0:
        parser.error("The installation byte limit must be positive")
    try:
        result = build(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, "Packaging failed: " + str(error) + "\n")
    print(json.dumps({"manifest": str(args.output / "UI23-delivery-manifest.json"),
                      "storage_target_met": result["animated_storage_target_met"],
                      "archives": result["archives"]}, ensure_ascii=False))


if __name__ == "__main__":
    main()
