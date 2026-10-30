#!/usr/bin/env python3
"""Package matching private calculator resources, with an optional browser player."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import zipfile

from build_web import ROOT, package_runtime
from split_content import language_resource_version, reconstruct, LANGUAGES
from storage_parts import PART_FILES, join_parts
from release_content_guard import validate_release_content


def resource_version(path):
    try:
        return language_resource_version(path.read_bytes())
    except ValueError as error:
        raise ValueError('Expected a valid NSL1/NSL2/NSL3 language resource: ' + str(path)) from error


def readme(info):
    shared = ('data/nSpire2.dat.tns         公共资源索引 / artwork index\n' +
              ''.join('data/'+name+'\n' for name in PART_FILES)) if info.get('multipart_artwork') else \
             'data/nSpire2.dat.tns         公共图片与动画 / shared artwork and animation\n'
    if info.get('local_checkpoints'):
        sources = (f"Runtime source checkpoint (local): {info['runtime_checkpoint']}\n"
                   f"Resource converter checkpoint (local): {info['resource_checkpoint']}\n"
                   "These source checkpoints have not been published to GitHub.\n")
    else:
        sources = (f"Runtime source: https://github.com/maxmixalex/slay-the-nspire-2/tree/{info['runtime_checkpoint']}\n"
                   f"Resource converter: https://github.com/maxmixalex/slay-the-nspire-2/tree/{info['resource_checkpoint']}\n")
    return f"""Slay the Nspire 2 — nSpire 0.1.0 (StS II v0.111.0) — {info['stage']}

安装 / Installation
首次安装：将 nSpire2.tns 与 data、state 文件夹一起复制到计算器上的游戏目录，打开 nSpire2.tns。
Copy nSpire2.tns and the data/state folders into the game's calculator directory.
Open nSpire2.tns. A color TI-Nspire with Ndless is required.

nSpire2.tns                  程序 / launcher
{shared.rstrip()}
data/nSpire2.eng.dat.tns     English
data/nSpire2.zhs.dat.tns     简体中文
state/                      局面、历史、设置、语言和档位 / saves and settings

只安装需要的语言文件。设置中可切换已安装语言；语言选择全局保存。
公共资源索引和全部用途包必须来自同一版本并一起安装，不能按用途删减。
升级前退出游戏，保留已有 state 文件夹及其中的档案，不要用包内的空 state 覆盖它。
删除游戏目录中旧的整个 data 文件夹，然后传入本包的完整 data 文件夹并替换 nSpire2.tns。
可删除新 data 中不需要的语言文件，但必须保留配套的新索引和全部用途包。
这同样适用于旧分包版升级，可避免传输时同时暂存旧大文件和新资源。
等全部配套文件传输完成后再启动。可选语言只传需要的几种，不必安装全部十六种。
首次启动按系统语言选择；对应语言缺失或无法加载时尝试英语，再选择首个可用语言。
主菜单左上角可切换三个档案，各自保存局面、历史与游戏设置。
程序会创建缺失的 state 文件夹。所有保存、临时文件、备份和诊断文件均写入 state。
Install only the desired language files in data. Settings lists installed languages.
Install the matching artwork index and every concern file together. They are all required.
Exit the game before updating; keep existing files in state.
Do not overwrite the existing state folder with the archive's empty state folder.
Replace the entire old data folder with this archive's matching data folder,
and replace nSpire2.tns. You may omit unwanted locale files, but keep the
matching index and every concern file.
This also applies to updates from an older multipart installation and avoids
temporarily storing both old large files and their replacements during transfer.
Start only after the complete matching set is present.
Install only the additional languages you need; all sixteen are not required.
First launch follows the system language, then English, then the first usable language.
Missing or unusable language resources fall back through the same sequence. Startup errors are English.
The top-left profile button selects one of three independent run/history/settings profiles.
Language is shared. The launcher creates state if necessary; all mutable files stay there.

操作 / Controls
触摸板四边导航，中心点击；数字 1–9、0 选择第 1–10 张手牌。
Enter 指向/确认；左右选择目标；Esc 返回/暂停。
Tab 地图；Doc 牌组；Home/On 抽牌堆；Del 弃牌堆；Var 消耗牌堆。
Menu 结束回合；Shift 按住查看选牌界面下方的战场；Ctrl 在纯键盘模式下聚焦顶部栏。
I 详情/地图图例，C 百科，V 状态，P 药水，R 遗物，O 读取。
暂停菜单可保存或退出；两套键位可在设置中分别修改。
制作人员名单中按住空格键加速滚动。
Touchpad edges navigate; its centre clicks. Digits 1–9 and 0 select hand slots 1–10.
Enter aims/confirms; arrows choose targets; Esc returns or pauses.
Tab map; Doc deck; Home/On draw pile; Del discard; Var exhaust; Menu ends the turn.
Hold Shift to peek below a card selection. Ctrl focuses the top bar in keyboard-only mode.
I details/map legend; C compendium; V statuses; P potions; R relics; O load.
Use the pause menu to save or exit. Settings has two separately remappable key banks.
Hold Space to speed up the credits.

原版控制台 / Developer console
每个构建均包含控制台，不需要另一个启动器或编译开关。
Flag 打开/关闭；输入命令后 Enter 执行；Tab 补全；方向键编辑/浏览历史；Esc 关闭。
help 查看命令，help card 查看语法，例如 gold 10、card STRIKE_IRONCLAD、room Shop。
调试命令本身不自动保存，主动保存会写入修改后的局面。
Every build includes the console. Flag toggles it; Enter executes; Tab completes;
arrows edit or browse history; Esc closes. Use help for available commands.
Commands modify the current game without autosaving; an explicit Save persists the change.

版本 / Build
Reference: v0.111.0 / 41cef1ea
Resource format: NSP2/{info['resource_format']}; save format: v{info['save_version']}
Installed program/resources: {sum(info['files'].values()):,} bytes before saves
{sources.rstrip()}
UI and calculator performance are validated using Firebird; physical-device timing is separate.
Source and third-party notices are included under data/licenses. Original game assets belong
 to Mega Crit and the respective rights holders.
"""


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('launcher', 'common', 'eng', 'zhs', 'output'):
        p.add_argument('--' + name, type=Path, required=True)
    p.add_argument('--wasm',type=Path)
    p.add_argument('--calculator-only',action='store_true',help='Do not build or package a browser player')
    p.add_argument('--local-checkpoints',action='store_true',help='Record local source receipts without claiming published Git refs')
    p.add_argument('--optional-locales',type=Path,help='Verified language editions directory; package additional locales separately')
    p.add_argument('--runtime-checkpoint', required=True)
    p.add_argument('--resource-checkpoint', required=True)
    p.add_argument('--stage', default='ui20', help='Release suffix; UI20 and newer require the completed UI20 content')
    args = p.parse_args()
    if not re.fullmatch('[A-Za-z0-9][A-Za-z0-9_-]*', args.stage):
        p.error('Stage must contain only letters, digits, underscores or hyphens')
    for value in (args.runtime_checkpoint, args.resource_checkpoint):
        if not re.fullmatch('[0-9a-f]{40}', value):
            p.error('Checkpoints must be full Git commit IDs')
    paths = {'nSpire2.tns': args.launcher, 'data/nSpire2.dat.tns': args.common,
             'data/nSpire2.eng.dat.tns': args.eng, 'data/nSpire2.zhs.dat.tns': args.zhs}
    common_bytes=args.common.read_bytes()
    multipart=common_bytes[:4]==b'NSM1'
    if multipart:
        parts={name:args.common.parent/name for name in PART_FILES}
        for name,path in parts.items():
            if not path.is_file():p.error('Artwork concern file is missing: '+str(path))
            paths['data/'+name]=path
        common_bytes=join_parts(common_bytes,{name:path.read_bytes() for name,path in parts.items()})
    if not args.calculator_only and not args.wasm:
        p.error('Provide --wasm or choose --calculator-only')
    for path in (*paths.values(), *([args.wasm] if args.wasm else [])):
        if not path.is_file():
            p.error('Input file is missing: ' + str(path))
    args.output.mkdir(parents=True, exist_ok=True)
    version = resource_version(args.eng)
    if resource_version(args.zhs) != version:
        p.error('English and Chinese resources must use the same format version')
    content_validation = None
    for code,path in (('eng',args.eng),('zhs',args.zhs)):
        logical = reconstruct(common_bytes,path.read_bytes(),code)
        if code == 'eng':
            content_validation = validate_release_content(logical, args.stage)
        del logical
    info = dict(version='0.1.0', reference='v0.111.0 / 41cef1ea', resource_format=version, save_version=17,
                progress_version=9, settings_version=6, calculator_run_storage='NSZ1 or raw NSV1',
                calculator_history_storage='NSZ1 or raw NSH1-9',
                stage=args.stage,
                runtime_checkpoint=args.runtime_checkpoint,
                resource_checkpoint=args.resource_checkpoint,
                developer_console=True, installation_layout="data/state",multipart_artwork=multipart,
                locale_storage='NSL3' if args.eng.read_bytes()[:4]==b'NSL3' else 'NSL1/NSL2',
                calculator_only=args.calculator_only,local_checkpoints=args.local_checkpoints,
                files={name: path.stat().st_size for name, path in paths.items()})
    if content_validation is not None:
        info['required_content_validation'] = content_validation
    stem = f'Slay-the-Nspire-2-0.1.0-{args.stage}-bilingual'
    archive = args.output / (stem + '-calculator.zip')
    offline = args.output / (stem + '-offline.html')
    texts = {'data/README.txt': readme(info).encode('utf-8'),
             'data/BUILD-INFO.json': (json.dumps(info, ensure_ascii=False, indent=2) + '\n').encode('utf-8')}
    for name in ('COPYING', 'NOTICE.md', 'licenses/Go-BSD-3-Clause.txt',
                 'licenses/OkHSV-Godot-MIT.txt', 'licenses/Noto-Boot-OFL.txt','licenses/NotoEmoji-OFL.txt','licenses/NotoSansSymbols2-OFL.txt'):
        paths["data/licenses/" + Path(name).name] = ROOT / name
    paths['data/licenses/Unicode-License.txt'] = ROOT / 'third_party/unicode/LICENSE'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as out:
        out.writestr('state/', b'')
        for name, path in paths.items():
            out.write(path, name)
        for name, data in texts.items():
            out.writestr(name, data)
    # Exercise actual archive reads, rather than relying only on creation.
    with zipfile.ZipFile(archive) as check:
        if set(check.namelist()) != set(paths) | set(texts) | {"state/"}:
            raise ValueError('Unexpected delivery members')
        for name, path in paths.items():
            if check.read(name) != path.read_bytes():
                raise ValueError('Archive contents differ: ' + name)
        for name, data in texts.items():
            if check.read(name) != data:
                raise ValueError('Archive text differs: ' + name)
    if not args.calculator_only:
        package_runtime(destination=offline, common_path=args.common, eng_path=args.eng,
                        zhs_path=args.zhs, wasm_path=args.wasm,
                        source_checkpoint=args.runtime_checkpoint,
                        local_checkpoints=args.local_checkpoints)
    optional=None
    if args.optional_locales:
        extra={'data/nSpire2.'+code+'.dat.tns':args.optional_locales/('nSpire2.'+code+'.dat.tns')
               for code in LANGUAGES if code not in ('eng','zhs')}
        if len(extra)!=14 or any(resource_version(path)!=version for path in extra.values()):
            raise ValueError('Expected the fourteen additional matching original locales')
        optional=args.output/f'Slay-the-Nspire-2-0.1.0-{args.stage}-optional-languages.zip'
        instructions=('只复制需要的语言包，放入配套程序的 data 文件夹，在主菜单设置切换语言。\n'
                      'Copy only desired language files into the matching installation data folder. Select in main-menu Settings.\n'
                      'English and Simplified Chinese are included in the main calculator archive.\n'+
                      ''.join(name+'  '+str(path.stat().st_size)+' bytes\n' for name,path in extra.items())).encode()
        with zipfile.ZipFile(optional,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as out:
            out.writestr('data/README-languages.txt',instructions)
            for name,path in extra.items():out.write(path,name)
        with zipfile.ZipFile(optional) as check:
            if set(check.namelist())!=set(extra)|{'data/README-languages.txt'} or check.read('data/README-languages.txt')!=instructions:
                raise ValueError('Optional locale archive members differ')
            for name,path in extra.items():
                if check.read(name)!=path.read_bytes():raise ValueError('Optional locale differs: '+name)
    print(json.dumps(dict(calculator=str(archive), calculator_bytes=archive.stat().st_size,
                          offline=str(offline) if not args.calculator_only else None,
                          offline_bytes=offline.stat().st_size if not args.calculator_only else 0,
                          optional_locales=str(optional) if optional else None,
                          installed_bytes=sum(info['files'].values()), members=len(paths) + len(texts) + 1)))


if __name__ == '__main__':
    main()
