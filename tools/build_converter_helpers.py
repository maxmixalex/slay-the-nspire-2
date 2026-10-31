#!/usr/bin/env python3
"""Build private converter helpers that need no player-side SDK or C++ compiler."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def build(args):
    from world_enemy_assets import _FASTNOISE_CALLER
    output = args.output.resolve()
    if output == ROOT or ROOT in output.parents:
        raise ValueError("Private runtime helper outputs must stay outside the source repository")
    output.mkdir(parents=True, exist_ok=True)
    runtime = args.spine_runtime.resolve(strict=True)
    header = args.fastnoise_header.resolve(strict=True)
    work = output / "build" / args.rid
    work.mkdir(parents=True, exist_ok=True)
    target = output / args.rid
    target.mkdir(exist_ok=True)
    env = os.environ.copy()
    env.setdefault("DOTNET_CLI_HOME", str(work / "dotnet-home"))
    env.setdefault("NUGET_PACKAGES", str(work / "nuget-packages"))
    env["DOTNET_NOLOGO"] = "1"
    env["DOTNET_SKIP_FIRST_TIME_EXPERIENCE"] = "1"
    subprocess.run([str(args.dotnet), "publish", str(ROOT / "tools/spine_sample_adapter/SpineSampleAdapter.csproj"),
                    "-c", "Release", "-r", args.rid, "--self-contained", "true", "-o", str(target),
                    "-p:SpineRuntimePath=" + str(runtime),
                    "-p:BaseIntermediateOutputPath=" + str(work / "obj") + os.sep,
                    "-p:BaseOutputPath=" + str(work / "bin") + os.sep],
                   check=True, env=env)
    noise_source = work / "noise.cpp"
    noise_source.write_text(_FASTNOISE_CALLER)
    shutil.copyfile(header, work / "FastNoiseLite.h")
    windows = args.rid.startswith("win-")
    mac = args.rid.startswith("osx-")
    suffix = ".dll" if windows else ".dylib" if mac else ".so"
    library = target / ("nspire_noise" + suffix)
    command = [args.cxx, "-std=c++17", "-O2", "-fwrapv", "-ffp-contract=off"]
    if not windows:
        command.append("-fPIC")
    command.extend(["-dynamiclib" if mac else "-shared", str(noise_source), "-o", str(library)])
    if windows:
        command.extend(["-static-libgcc", "-static-libstdc++"])
    subprocess.run(command, check=True)
    adapter = target / ("SpineSampleAdapter.exe" if windows else "SpineSampleAdapter")
    if not adapter.is_file() or not library.is_file():
        raise ValueError("A helper output is missing")
    receipt = {"schema": 1, "rid": args.rid, "self_contained_adapter": True,
               "player_needs_dotnet_sdk": False, "player_needs_cxx_compiler": False,
               "spine_runtime_sha256": hashlib.sha256(runtime.read_bytes()).hexdigest(),
               "fastnoise_header_sha256": hashlib.sha256(header.read_bytes()).hexdigest(),
               "helpers": {p.name: {"bytes": p.stat().st_size,
                                      "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
                           for p in (adapter, library)}}
    (target / "helper-build.json").write_text(json.dumps(receipt, indent=2) + "\n")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("spine-runtime", "fastnoise-header", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--dotnet", type=Path, default=Path("dotnet"))
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--rid", default="linux-x64", choices=("linux-x64", "win-x64", "osx-x64", "osx-arm64"))
    args = parser.parse_args()
    try:
        receipt = build(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, "Helper build failed: " + str(error) + "\n")
    print(json.dumps(receipt))


if __name__ == "__main__":
    main()
