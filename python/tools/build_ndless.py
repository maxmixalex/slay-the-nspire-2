#!/usr/bin/env python3
"""Build the freestanding ARM926 Ndless launcher using LLVM (no network)."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
import argparse
import shutil
import subprocess
from pathlib import Path

from nspire_converter.paths import ROOT
from nspire_converter.zehn import build

# Keep renderers and hot kernels at -O2. Event rules and input dispatch use
# -Oz; progress and run/card generation use -Os, keeping 64-bit
# shifts inline. Map generation, renderers and combat kernels remain -O2.
# Every release and decoder-validation entry uses this identical policy.
SIZE_OPTIMIZATION = {
    "cpp/src/original_events.cpp": "-Oz",
    "cpp/src/ui_progress_codec.cpp": "-Os",
    "cpp/src/legacy_progress.cpp": "-Os",
    "cpp/src/ui_input.cpp": "-Oz",
    "cpp/src/pointer.cpp": "-Oz",
    "cpp/src/run.cpp": "-Os",
    "cpp/src/world_card_factory.cpp": "-Os",
    "cpp/src/character_progress.cpp": "-Os",
    "cpp/src/profile_input.cpp": "-Oz",
    "cpp/src/crystal_input.cpp": "-Oz",
    "cpp/src/collection_input.cpp": "-Oz",
    "cpp/src/source_version.cpp": "-Os",
    "cpp/src/world_choice_input.cpp": "-Oz",
    "cpp/src/run_modes_input.cpp": "-Oz",
    "cpp/src/combat_selection_input.cpp": "-Oz",
    "cpp/src/item_inspect_input.cpp": "-Oz",
    "cpp/src/card_grid_input.cpp": "-Oz",
    "cpp/src/card_inspect_input.cpp": "-Oz",
    "cpp/src/settings_input.cpp": "-Oz",
    "cpp/src/physical_input.cpp": "-Oz",
}
SIZE_OPTIMIZED_SOURCES = tuple(SIZE_OPTIMIZATION)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Launcher output path")
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=ROOT / "build" / "ndless",
        help="Directory for intermediate objects, ELF and link map",
    )
    parser.add_argument(
        "--size-event-rules",
        action="store_true",
        default=True,
        help=(
            "Compatibility flag: release builds use the source-specific -Oz/-Os "
            "policy for event/input/progress code; renderers remain -O2"
        ),
    )
    parser.add_argument(
        "--profile-neow-layers",
        action="store_true",
        help=("Developer-only reuse of existing timing slots for Neow layers; not a release build"),
    )
    parser.add_argument(
        "--trace-io",
        action="store_true",
        help=("Developer-only bounded OS I/O trace; not a release/timing acceptance build"),
    )
    parser.add_argument(
        "--compression",
        choices=("zlib", "zopfli"),
        default="zlib",
        help=(
            "Use zopfli for compact release packaging; requires the Poetry build dependency group"
        ),
    )
    args = parser.parse_args()
    clang = shutil.which("clang++") or shutil.which("clang++-18")
    link = shutil.which("ld.lld") or shutil.which("ld.lld-18")
    if not clang or not link:
        raise SystemExit("Install LLVM with clang++ and ld.lld, and put both on PATH.")
    directory = args.build_dir
    directory.mkdir(parents=True, exist_ok=True)
    sources = [
        "cpp/" + line.strip()
        for line in (ROOT / "cpp/core_sources.txt").read_text().splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    ]
    sources.extend(
        ("cpp/src/freestanding.cpp", "cpp/platform/ndless/main.cpp", "cpp/platform/ndless/entry.S")
    )
    flags = [
        "--target=arm-none-eabi",
        "-mcpu=arm926ej-s",
        "-marm",
        "-mfloat-abi=soft",
        "-mno-unaligned-access",
        "-O2",
        "-ffreestanding",
        "-fno-builtin",
        "-fno-exceptions",
        "-fno-rtti",
        "-fno-threadsafe-statics",
        "-fno-stack-protector",
        "-fno-unwind-tables",
        "-fno-asynchronous-unwind-tables",
        "-ffunction-sections",
        "-fdata-sections",
        "-fstack-usage",
        "-Wframe-larger-than=8192",
        "-I" + str(ROOT / "cpp/include"),
    ]
    if args.trace_io:
        flags.append("-DSPIRE_DEVICE_IO_TRACE=1")
    if args.profile_neow_layers:
        flags.append("-DSPIRE_NEOW_LAYER_PROFILE=1")
    objects = []
    for i, source in enumerate(sources):
        out = directory / f"{i}.o"
        objects.append(str(out))
        source_flags = [SIZE_OPTIMIZATION[source]] if source in SIZE_OPTIMIZATION else []
        if source.endswith(".c"):
            # The unmodified SDK is C. Its tiny compatibility headers must not
            # shadow native/system headers or the game's C++ translation units.
            options = [
                "-x",
                "c",
                "-std=c99",
                "-I" + str(ROOT / "cpp/platform/decoder_headers"),
            ]
            compile_flags = [
                f
                for f in flags
                if f not in ("-fno-exceptions", "-fno-rtti", "-fno-threadsafe-statics")
            ]
        else:
            options = (
                ["-std=c++17", "-I" + str(ROOT / "cpp/third_party/lzma2")]
                if source.endswith(".cpp")
                else []
            )
            if source == "cpp/src/lzma2_block.cpp":
                options += ["-I" + str(ROOT / "cpp/platform/decoder_headers")]
            compile_flags = flags
        subprocess.run(
            [
                clang,
                *compile_flags,
                *source_flags,
                *options,
                "-c",
                str(ROOT / source),
                "-o",
                str(out),
            ],
            check=True,
        )
    elf = directory / "nSpire2.elf"
    # Clang emits address-significance metadata. Safe ICF can share identical
    # unobserved function bodies while preserving distinct significant
    # addresses; the unrestricted "all" mode is deliberately not enabled.
    subprocess.run(
        [
            link,
            "-T",
            str(ROOT / "cpp/platform/ndless/link.ld"),
            "--gc-sections",
            "--icf=safe",
            "--emit-relocs",
            "-Map=" + str(directory / "nSpire2.map"),
            *objects,
            "-o",
            str(elf),
        ],
        check=True,
    )
    output = args.output or ROOT / "dist" / "nSpire2.tns"
    output.parent.mkdir(parents=True, exist_ok=True)
    build(elf, output, args.compression)


if __name__ == "__main__":
    main()
