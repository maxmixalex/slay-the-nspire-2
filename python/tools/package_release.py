#!/usr/bin/env python3
"""Package matching full, reduced and optional-language calculator installations."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

from nspire_converter import __version__
from nspire_converter.paths import ROOT
from nspire_converter.reference_inventory import verify_release
from nspire_converter.source_version import FILE_NAME, encode_version, installed_version
from nspire_converter.split_content import LANGUAGES, language_resource_version, reconstruct
from nspire_converter.storage_parts import PART_FILES, join_parts


def instructions(profile: str, game_version: str) -> bytes:
    edition = "完整动画 / full animation" if profile == "full" else "精简动画 / reduced animation"
    return f"""nSpire {__version__} (StS II v{game_version}) — {edition}

安装 / Install
将 nSpire2.tns 与完整 data 文件夹复制到计算器同一目录，使用 Ndless 打开 nSpire2.tns。
Copy nSpire2.tns and the complete data folder into one calculator directory and open nSpire2.tns with Ndless.

升级前备份 state 文件夹；替换旧 data 和 nSpire2.tns，保留 state 中的存档。
Back up state before upgrading. Replace data and nSpire2.tns; keep the existing state folder.

中英双语默认包含。额外语言包的 {profile}/data 配本版本，在设置中选择所复制的语言。
English and Simplified Chinese are included. Optional locales under {profile}/data match this edition; select them in Settings.

方向键导航，触摸板移动及点击，Enter确认，Esc返回或暂停。1–9/0选牌，Menu结束回合。
Arrows navigate; the touchpad moves/clicks; Enter confirms; Esc goes back/pauses. 1–9/0 select cards; Menu ends the turn.
Tab地图，Doc牌组，Home/On抽牌堆，Del弃牌堆，Var消耗牌堆。设置可调整按键及触摸轻点。
Tab opens the map; Doc the deck; Home/On the draw pile; Del the discard pile; Var the exhaust pile. Settings includes key bindings and tap to click.

从拥有的原版安装运行配套转换器，可用 --no-animations 或 --languages all。
Run the matching converter in your owned game installation. Use --no-animations or --languages all as needed.

音频、多人和在线服务未实现。原版素材归 Mega Crit 及各权利人所有。
Audio, multiplayer and online services are not implemented. Original assets belong to Mega Crit and their respective rights holders.
https://github.com/maxmixalex/slay-the-nspire-2
""".encode("utf-8")


def edition_files(directory: Path, profile: str) -> dict[str, Path]:
    common_path = directory / "nSpire2.dat.tns"
    common = common_path.read_bytes()
    files = {"data/nSpire2.dat.tns": common_path}
    if common[:4] == b"NSM1":
        parts = {name: directory / name for name in PART_FILES}
        common = join_parts(common, {name: path.read_bytes() for name, path in parts.items()})
        files.update({"data/" + name: path for name, path in parts.items()})
    logical_version: int | None = None
    for code in LANGUAGES:
        path = directory / f"nSpire2.{code}.dat.tns"
        localized = path.read_bytes()
        version = language_resource_version(localized)
        if logical_version is not None and logical_version != version:
            raise ValueError(f"Mixed resource formats in {directory}")
        logical_version = version
        logical = reconstruct(common, localized, code)
        if int.from_bytes(logical[24:28], "little") != 8707:
            raise ValueError(f"The {code} resources lack the complete character catalog")
        if code == "eng":
            from nspire_converter.character_unlock_assets import validate
            from nspire_converter.coalesce_tile_resources import view

            source = view(logical)
            validate(source, source.image_raw)
        reduced = (
            len(logical) >= 40
            and struct.unpack_from("<I", logical, 4)[0] >= 46
            and logical[36:40] == b"NAF1"
        )
        if reduced != (profile == "reduced"):
            raise ValueError(
                f"{code} resources do not match the requested {profile} animation profile"
            )
        if code in ("eng", "zhs"):
            files[f"data/nSpire2.{code}.dat.tns"] = path
    return files


def notices() -> dict[str, Path]:
    files = {"data/licenses/LICENSE": ROOT / "LICENSE"}
    source = ROOT / "cpp/third_party/notices"
    for path in sorted(source.glob("*")):
        if path.is_file():
            files["data/licenses/" + path.name] = path
    files["data/licenses/Unicode-License.txt"] = ROOT / "cpp/third_party/unicode/LICENSE"
    for path in files.values():
        if not path.is_file():
            raise ValueError(f"Required license is missing: {path}")
    return files


def write_archive(
    path: Path,
    files: dict[str, Path],
    generated: dict[str, bytes],
    *,
    maximum_bytes: int | None = None,
) -> dict[str, object]:
    installed = sum(p.stat().st_size for p in files.values()) + sum(map(len, generated.values()))
    if maximum_bytes is not None and installed > maximum_bytes:
        raise ValueError(f"Installation is {installed} bytes, above the {maximum_bytes}-byte limit")
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(".tmp")
    with zipfile.ZipFile(temporary, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, source in sorted(files.items()):
            archive.write(source, name)
        for name, data in sorted(generated.items()):
            archive.writestr(name, data)
    with zipfile.ZipFile(temporary) as archive:
        if set(archive.namelist()) != set(files) | set(generated):
            raise ValueError("Package member list changed")
        for name, source in files.items():
            if archive.read(name) != source.read_bytes():
                raise ValueError(f"Package readback differs: {name}")
        for name, data in generated.items():
            if archive.read(name) != data:
                raise ValueError(f"Package readback differs: {name}")
    temporary.replace(path)
    return {
        "path": str(path),
        "archive_bytes": path.stat().st_size,
        "installed_bytes": installed,
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
    }


def package(
    game: Path, full: Path, reduced: Path, launcher: Path, output: Path
) -> dict[str, object]:
    verify_release(game)
    version = installed_version(game)
    metadata = encode_version(version)
    if not launcher.is_file():
        raise ValueError(f"Launcher is missing: {launcher}")
    receipts: dict[str, object] = {"port_version": __version__, "source_game_version": version}
    languages: dict[str, Path] = {}
    for profile, directory in (("full", full), ("reduced", reduced)):
        files = edition_files(directory, profile)
        files.update(notices())
        files["nSpire2.tns"] = launcher
        generated = {
            "data/" + FILE_NAME: metadata,
            "data/README.txt": instructions(profile, version),
        }
        receipts[profile] = write_archive(
            output / f"nSpire-{__version__}-{profile}.zip",
            files,
            generated,
            maximum_bytes=70_000_000 if profile == "full" else None,
        )
        for code in LANGUAGES:
            if code not in ("eng", "zhs"):
                languages[f"{profile}/data/nSpire2.{code}.dat.tns"] = (
                    directory / f"nSpire2.{code}.dat.tns"
                )
    receipts["languages"] = write_archive(
        output / f"nSpire-{__version__}-languages.zip",
        languages,
        {
            "README.txt": b"Copy only desired locale files from full/data or reduced/data into the matching installation's data folder. Select the language in Settings.\n"
        },
    )
    return receipts


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("game", "full", "reduced", "launcher", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    try:
        result = package(args.game, args.full, args.reduced, args.launcher, args.output)
    except (OSError, ValueError) as error:
        parser.exit(1, f"Packaging failed: {error}\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
