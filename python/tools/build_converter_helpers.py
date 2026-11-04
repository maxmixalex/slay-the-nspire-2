#!/usr/bin/env python3
"""Build private converter helpers that need no player-side SDK or C++ compiler."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from nspire_converter.paths import ROOT


def verify_helpers(directory: Path) -> None:
    """Load both native helpers without the developer's runtime search paths."""
    directory = directory.resolve(strict=True)
    adapter = directory / ("SpineSampleAdapter.exe" if os.name == "nt" else "SpineSampleAdapter")
    suffix = ".dll" if os.name == "nt" else ".dylib" if sys.platform == "darwin" else ".so"
    library = directory / ("nspire_noise" + suffix)
    environment = os.environ.copy()
    for name in ("PYTHONPATH", "PYTHONHOME", "LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH"):
        environment.pop(name, None)
    environment["PATH"] = (
        str(Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32")
        if os.name == "nt"
        else "/usr/bin:/bin"
    )
    probe = (
        "import ctypes, math, sys\n"
        "library = ctypes.CDLL(sys.argv[1])\n"
        "library.nspire_decim_noise.argtypes = [ctypes.c_float]\n"
        "library.nspire_decim_noise.restype = ctypes.c_float\n"
        "if not math.isfinite(library.nspire_decim_noise(0.25)):\n"
        "    raise ValueError('Native noise helper returned a non-finite sample')\n"
    )
    with tempfile.TemporaryDirectory(prefix="nspire-native-helpers-") as working_directory:
        subprocess.run(
            [sys.executable, "-I", "-c", probe, str(library)],
            cwd=working_directory,
            env=environment,
            check=True,
            timeout=30,
        )
        result = subprocess.run(
            [str(adapter)],
            input='{"schema_version":2}\n',
            text=True,
            capture_output=True,
            cwd=working_directory,
            env=environment,
            check=True,
            timeout=30,
        )
        reply = json.loads(result.stdout)
        if reply.get("ok") is not False or reply.get("error") != "Unsupported adapter protocol":
            raise ValueError("Native Spine adapter did not answer its protocol check")


def build(args):
    from nspire_converter.ancient_turbulence import _TURBULENCE_CALLER
    from nspire_converter.neow_water_post import _NEOW_NOISE_CALLER
    from nspire_converter.random_character_assets import _RANDOM_NOISE_CALLER
    from nspire_converter.vantom_vfx_assets import _VANTOM_NOISE_CALLER
    from nspire_converter.world_enemy_assets import _FASTNOISE_CALLER

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
    subprocess.run(
        [
            str(args.dotnet),
            "publish",
            str(ROOT / "python/dotnet/spine_sample_adapter/SpineSampleAdapter.csproj"),
            "-c",
            "Release",
            "-r",
            args.rid,
            "--self-contained",
            "true",
            "-o",
            str(target),
            "-p:SpineRuntimePath=" + str(runtime),
            "-p:BaseIntermediateOutputPath=" + str(work / "obj") + os.sep,
            "-p:BaseOutputPath=" + str(work / "bin") + os.sep,
        ],
        check=True,
        env=env,
    )
    noise_source = work / "noise.cpp"
    noise_source.write_text(
        _FASTNOISE_CALLER
        + "\n"
        + _VANTOM_NOISE_CALLER
        + "\n"
        + _RANDOM_NOISE_CALLER
        + "\n"
        + _TURBULENCE_CALLER
        + "\n"
        + _NEOW_NOISE_CALLER
    )
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
        # MinGW's static C++ runtime can still depend on a shared winpthread.
        # Link its compiler runtimes statically into this DLL as well.
        command.append("-static")
    subprocess.run(command, check=True)
    adapter = target / ("SpineSampleAdapter.exe" if windows else "SpineSampleAdapter")
    if not adapter.is_file() or not library.is_file():
        raise ValueError("A helper output is missing")
    # Legacy conversion stages use these explicit paths as cache inputs. They
    # remain private bundle files, while the executable avoids first-run builds.
    shutil.copyfile(runtime, target / "OurNotes.Spine.Runtime.dll")
    shutil.copyfile(header, target / "FastNoiseLite.h")
    license_path = runtime.parent / "Spine-Runtimes-LICENSE"
    if not license_path.is_file():
        raise ValueError("The supplied runtime must include Spine-Runtimes-LICENSE beside its DLL")
    shutil.copyfile(license_path, target / "Spine-Runtimes-LICENSE")
    receipt = {
        "schema": 1,
        "rid": args.rid,
        "self_contained_adapter": True,
        "player_needs_dotnet_sdk": False,
        "player_needs_cxx_compiler": False,
        "spine_runtime_sha256": hashlib.sha256(runtime.read_bytes()).hexdigest(),
        "fastnoise_header_sha256": hashlib.sha256(header.read_bytes()).hexdigest(),
        "helpers": {
            p.name: {
                "bytes": p.stat().st_size,
                "sha256": hashlib.sha256(p.read_bytes()).hexdigest(),
            }
            for p in (adapter, library)
        },
    }
    (target / "helper-build.json").write_text(json.dumps(receipt, indent=2) + "\n")
    verify_helpers(target)
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("spine-runtime", "fastnoise-header", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--dotnet", type=Path, default=Path("dotnet"))
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument(
        "--rid", default="linux-x64", choices=("linux-x64", "win-x64", "osx-x64", "osx-arm64")
    )
    args = parser.parse_args()
    try:
        receipt = build(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, "Helper build failed: " + str(error) + "\n")
    print(json.dumps(receipt))


if __name__ == "__main__":
    main()
