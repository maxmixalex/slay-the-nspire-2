#!/usr/bin/env python3
"""Compile the complete converter with Nuitka for the current operating system."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

from nspire_converter import __version__
from nspire_converter.inventory import inventory
from nspire_converter.paths import PACKAGE_ROOT, ROOT


def build(helpers: Path, output: Path, work: Path, *, mode: str = "onefile") -> Path:
    """Compile recipe modules and embed the native helpers and public input tables."""
    from nspire_converter.converter_arm_validation import source_inventory, verify
    from nspire_converter.locale_profile import validate_generated

    helpers = helpers.resolve(strict=True)
    output, work = output.resolve(), work.resolve()
    output.mkdir(parents=True, exist_ok=True)
    work.mkdir(parents=True, exist_ok=True)
    validate_generated()
    verify(helpers)
    adapter = "SpineSampleAdapter.exe" if os.name == "nt" else "SpineSampleAdapter"
    noise = "nspire_noise" + (".dll" if os.name == "nt" else ".so")
    required = (
        adapter,
        noise,
        "OurNotes.Spine.Runtime.dll",
        "FastNoiseLite.h",
        "helper-build.json",
        "Spine-Runtimes-LICENSE",
        "nSpire2.elf",
        "arm-validation.json",
    )
    for name in required:
        if not (helpers / name).is_file():
            raise ValueError(f"Missing target helper: {name}")
    staging = work / "payload"
    staging.mkdir(exist_ok=True)
    bundled_helpers = staging / "converter_helpers"
    bundled_helpers.mkdir(exist_ok=True)
    for name in required:
        shutil.copy2(helpers / name, bundled_helpers / name)
    for name, data in (
        ("source-inventory.json", inventory()),
        ("arm-source-inventory.json", source_inventory()),
    ):
        (bundled_helpers / name).write_text(json.dumps(data, sort_keys=True), encoding="utf-8")
    # Managed assembly bytes identify sampling caches; the adapter already embeds
    # that assembly. Carry its identity as data, not as a second native library.
    (bundled_helpers / "OurNotes.Spine.Runtime.identity").write_bytes(
        (bundled_helpers / "OurNotes.Spine.Runtime.dll").read_bytes()
    )
    (bundled_helpers / "OurNotes.Spine.Runtime.dll").unlink()
    configuration = work / "helpers.nuitka-package.config.yml"
    configuration.write_text(
        json.dumps(
            [
                {
                    "module-name": "nspire_converter",
                    "dlls": [
                        {
                            "by_code": {
                                "filename_code": repr(str(bundled_helpers / adapter)),
                                "executable": "yes",
                            },
                            "dest_path": "converter_helpers",
                        },
                        {
                            "by_code": {
                                "filename_code": repr(str(bundled_helpers / noise)),
                            },
                            "dest_path": "converter_helpers",
                        },
                    ],
                },
                {
                    "module-name": "unicorn",
                    "dlls": [
                        {
                            "from_filenames": {"relative_path": "lib", "prefixes": [""]},
                            "dest_path": "unicorn/lib",
                        }
                    ],
                },
            ],
            indent=2,
        ),
        encoding="utf-8",
    )
    identity = hashlib.sha256(json.dumps(inventory(), sort_keys=True).encode()).hexdigest()[:16]
    executable_name = "nspire-converter" + (".exe" if os.name == "nt" else "")
    command = [
        sys.executable,
        "-X",
        "utf8",
        "-m",
        "nuitka",
        f"--mode={mode}",
        "--assume-yes-for-downloads",
        f"--output-dir={work / 'native'}",
        f"--output-filename={executable_name}",
        "--jobs=2",
        "--include-package=nspire_converter",
        "--include-package=unicorn",
        f"--user-package-configuration-file={configuration}",
        f"--include-data-dir={PACKAGE_ROOT / 'data'}=nspire_converter/data",
        f"--include-data-dir={ROOT / 'cpp/src'}=cpp/src",
        f"--include-data-dir={ROOT / 'cpp/include'}=cpp/include",
        f"--report={work / 'nuitka-report.xml'}",
        "--company-name=nSpire",
        "--product-name=nSpire Converter",
        f"--product-version={__version__}.0",
        f"--file-version={__version__}.0",
    ]
    # Register native helpers exclusively as binaries. Linux's extensionless
    # sampler would otherwise also enter the data list and conflict at packaging.
    command += [
        f"--include-data-files={path}=converter_helpers/{path.name}"
        for path in sorted(bundled_helpers.iterdir())
        if path.is_file() and path.name not in (adapter, noise)
    ]
    if mode == "onefile":
        command += [
            f"--onefile-tempdir-spec={{CACHE_DIR}}/nSpire/converter/{identity}",
            "--onefile-cache-mode=cached",
        ]
    if os.name == "nt":
        command.append("--mingw64")
    command.append(str(ROOT / "python/nspire_converter_main.py"))
    environment = os.environ.copy()
    environment["PYTHONPATH"] = (
        str(ROOT / "python/src") + os.pathsep + environment.get("PYTHONPATH", "")
    )
    subprocess.run(command, check=True, cwd=ROOT, env=environment)
    native = work / "native"
    if mode == "onefile":
        executable = output / executable_name
        shutil.copy2(native / executable_name, executable)
    else:
        distribution = native / "nspire_converter_main.dist"
        target = output / "nspire-converter"
        shutil.copytree(distribution, target, dirs_exist_ok=True)
        executable = target / executable_name
    smoke_environment = os.environ.copy()
    for key in (
        "PYTHONPATH",
        "PYTHONHOME",
        "NSPIRE_SPINE_ADAPTER",
        "NSPIRE_FASTNOISE_LIBRARY",
        "NSPIRE_SPINE_RUNTIME",
        "NSPIRE_FASTNOISE_HEADER",
        "NSPIRE_DOTNET",
    ):
        smoke_environment.pop(key, None)
    smoke_environment["PATH"] = (
        str(Path(os.environ.get("SystemRoot", "C:/Windows")) / "System32")
        if os.name == "nt"
        else "/usr/bin:/bin"
    )
    subprocess.run([str(executable), "--help"], check=True, env=smoke_environment)
    result = subprocess.run(
        [str(executable), "--encode-seed", "oldA"],
        check=True,
        capture_output=True,
        text=True,
        env=smoke_environment,
    )
    if result.stdout.strip() != "SEED64-EC8011CFFF0DFCC3":
        raise ValueError("Compiled seed encoder failed its known original-game vector")
    with tempfile.TemporaryDirectory(prefix="nspire-converter-smoke-") as directory:
        subprocess.run(
            [str(executable), "--self-test"], check=True, cwd=directory, env=smoke_environment
        )
    notices = output / "licenses"
    notices.mkdir(exist_ok=True)
    shutil.copy2(ROOT / "LICENSE", notices / "LICENSE")
    shutil.copy2(ROOT / "cpp/third_party/notices/ATTRIBUTIONS.txt", notices / "ATTRIBUTIONS.txt")
    shutil.copy2(helpers / "Spine-Runtimes-LICENSE", notices / "Spine-Runtimes-LICENSE")
    (notices / "FastNoiseLite-LICENSE.txt").write_text(
        (helpers / "FastNoiseLite.h").read_text(encoding="utf-8").split("#ifndef", 1)[0],
        encoding="utf-8",
    )
    (output / "README.txt").write_text(
        "Copy the converter into your owned Slay the Spire 2 installation and run it.\n"
        "Default: English + Simplified Chinese, full animation.\n"
        "Options: --no-animations, --languages all, --game PATH, --output PATH.\n"
        "Seed import: --encode-seed TEXT or --encode-seed-file FILE.\n"
        "No Python, .NET SDK or compiler installation is needed.\n",
        encoding="utf-8",
    )
    return executable


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("helpers", "output", "work"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--mode", choices=("onefile", "standalone"), default="onefile")
    args = parser.parse_args()
    try:
        executable = build(args.helpers, args.output, args.work, mode=args.mode)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Converter build failed: {error}\n")
    print(executable)


if __name__ == "__main__":
    main()
