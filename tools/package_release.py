#!/usr/bin/env python3
"""Package matching private calculator resources and an offline browser player."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import zipfile

from build_web import ROOT, package_runtime


def readme(info):
    return f'''Slay the Nspire 2 — 0.17 ui3 / 中文与 English

安装 / Installation
把下面四个文件放到计算器的同一文件夹，打开 nSpire2.tns：
Put these four files in one calculator folder, then open nSpire2.tns:
  nSpire2.tns          程序 / launcher
  nSpire2.common.tns   公共图片与动画 / shared artwork and animation
  nSpire2.eng.tns      英文 / English
  nSpire2.zhs.tns      中文 / Chinese
需要已安装 Ndless 的彩屏 TI-Nspire。单语言使用可只安装对应的语言包。
A color TI-Nspire with Ndless is required. Install either or both locale files.
双语安装时，4 选中文、6 选英文、Enter / 5 确认；后续启动记住该选择。
With both locales installed, 4 selects Chinese, 6 English, Enter / 5 confirms.
The choice is remembered and can be changed before loading on later launches.

升级与存档 / Updating and saves
请配套替换程序、公共包、已安装的语言包；保留 nSpire2.sav.tns 和语言设置。
Update the launcher, common pack and installed locales together. Keep your save
and language preference files. Both languages use the same gameplay save.
更新前备份存档。新程序可读取旧存档；旧程序可能无法读取更新后写出的存档。
Back up the save before upgrading. This launcher reads older saves; an older
launcher may reject a save subsequently written by this version.
如果系统替换大文件时提示空间不足，可先删除旧 common 资源文件，再传入新包。
If TI OS needs extra space while replacing the common file, remove the old
nSpire2.common.tns first, then transfer the new complete set. Keep your saves.
旧版 nSpire2.nsp.tns 完整资源包无需与新的公共/语言包同时保留。
The old complete nSpire2.nsp.tns pack is unnecessary alongside split resources.

操作 / Controls
方向键移动；Enter 确认；Space 结束回合或确认选牌；Esc 返回/暂停。
Tab 牌堆，I 详情，C 百科，V 状态，M 地图，P 药水，R 遗物，S 保存，O 读取。
Directions navigate; Enter confirms; Space ends a turn/confirms a selection;
Esc returns/pauses; Tab piles; I details; C compendium; V statuses; M map;
P potions; R relics; S saves; O loads. Touchpad clicking/dragging is supported.
离线 HTML 可直接用浏览器打开，在页面选择语言；S 保存、O 读取，存档保留在该浏览器中。
Open the companion offline HTML in a browser and choose a language on the page.
S saves and O loads; saves stay in that browser. No asset download or server connection is needed.

版本 / Build
参考游戏 / game reference: v0.111.0 / 41cef1ea
Resource format: NSP2/20; save format: v15
Installed files: {sum(info['files'].values()):,} bytes before saves
Runtime source: https://github.com/maxmixalex/slay-the-nspire-2/tree/{info['runtime_checkpoint']}
Resource converter: https://github.com/maxmixalex/slay-the-nspire-2/tree/{info['resource_checkpoint']}
程序源代码许可与第三方声明见 COPYING、NOTICE.md、licenses/。
See COPYING, NOTICE.md and licenses/ for source and third-party licenses.
原版游戏资源的权利属于 Mega Crit 及相应权利人。
Original game content belongs to Mega Crit and the respective rights holders.
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('launcher', 'common', 'eng', 'zhs', 'wasm', 'output'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--runtime-checkpoint', required=True)
    p.add_argument('--resource-checkpoint', required=True)
    args = p.parse_args()
    for value in (args.runtime_checkpoint, args.resource_checkpoint):
        if not re.fullmatch('[0-9a-f]{40}', value):
            p.error('Checkpoints must be full Git commit IDs')
    paths = {'nSpire2.tns': args.launcher, 'nSpire2.common.tns': args.common,
             'nSpire2.eng.tns': args.eng, 'nSpire2.zhs.tns': args.zhs}
    for path in (*paths.values(), args.wasm):
        if not path.is_file():
            p.error('Input file is missing: ' + str(path))
    args.output.mkdir(parents=True, exist_ok=True)
    info = dict(reference='v0.111.0 / 41cef1ea', resource_format=20, save_version=15,
                runtime_checkpoint=args.runtime_checkpoint,
                resource_checkpoint=args.resource_checkpoint,
                files={name: path.stat().st_size for name, path in paths.items()})
    stem = 'Slay-the-Nspire-2-0.17-ui3-bilingual'
    archive = args.output / (stem + '-calculator.zip')
    offline = args.output / (stem + '-offline.html')
    texts = {'README.txt': readme(info).encode('utf-8'),
             'BUILD-INFO.json': (json.dumps(info, ensure_ascii=False, indent=2) + '\n').encode('utf-8')}
    for name in ('COPYING', 'NOTICE.md', 'licenses/Go-BSD-3-Clause.txt',
                 'licenses/OkHSV-Godot-MIT.txt', 'licenses/Noto-Boot-OFL.txt'):
        paths[name] = ROOT / name
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as out:
        for name, path in paths.items():
            out.write(path, name)
        for name, data in texts.items():
            out.writestr(name, data)
    # Exercise actual archive reads, rather than relying only on creation.
    with zipfile.ZipFile(archive) as check:
        if set(check.namelist()) != set(paths) | set(texts):
            raise ValueError('Unexpected delivery members')
        for name, path in paths.items():
            if check.read(name) != path.read_bytes():
                raise ValueError('Archive contents differ: ' + name)
        for name, data in texts.items():
            if check.read(name) != data:
                raise ValueError('Archive text differs: ' + name)
    package_runtime(destination=offline, common_path=args.common, eng_path=args.eng,
                    zhs_path=args.zhs, wasm_path=args.wasm,
                    source_checkpoint=args.runtime_checkpoint)
    print(json.dumps(dict(calculator=str(archive), calculator_bytes=archive.stat().st_size,
                          offline=str(offline), offline_bytes=offline.stat().st_size,
                          installed_bytes=sum(info['files'].values()), members=len(paths) + len(texts))))


if __name__ == '__main__':
    main()
