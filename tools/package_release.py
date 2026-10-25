#!/usr/bin/env python3
"""Package matching private calculator resources, with an optional browser player."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import zipfile

from build_web import ROOT, package_runtime
from split_content import language_resource_version


def resource_version(path):
    try:
        return language_resource_version(path.read_bytes())
    except ValueError as error:
        raise ValueError('Expected a valid NSL1/NSL2 language resource: ' + str(path)) from error


def readme(info):
    text = f'''Slay the Nspire 2 — 0.17 {info['stage']} / 中文与 English

安装 / Installation
把下面四个文件放到计算器的同一文件夹，打开 nSpire2.tns：
Put these four files in one calculator folder, then open nSpire2.tns:
  nSpire2.tns          程序 / launcher
  nSpire2.common.tns   公共图片与动画 / shared artwork and animation
  nSpire2.eng.tns      英文 / English
  nSpire2.zhs.tns      中文 / Chinese
需要已安装 Ndless 的彩屏 TI-Nspire。单语言使用可只安装对应的语言包。
A color TI-Nspire with Ndless is required. Install either or both locale files.
启动播放原版制作组标志动画，按选择、取消或点击可跳过；设置中的“跳过开场标志”会记住选择。
The original studio logo plays on startup. Select, Cancel or a click skips it;
Settings can remember Skip Intro Logo. It does not replay when changing language.
双语安装可在主菜单设置中切换语言，之后会记住选择。
With both locales installed, change language in the main menu's Settings.
The choice is remembered across launches.
新档案首次选择“启程”时可选择是否启用原版教程；教程进度按档案独立记忆。
A new profile asks whether to enable tutorials on its first Embark. Tutorial
progress is remembered separately for each profile.

升级与存档 / Updating and saves
请配套替换程序、公共包、已安装的语言包；保留 sav、progress、settings、slots、lang 文件。
Update the launcher, common pack and installed locales together. Keep your run,
progress, settings, profile selector and language files, including .p2/.p3 files.
Both languages use the same three gameplay profiles.
主菜单左上角可以切换三个存档档位；每个档位独立保存局面、历史和游戏设置，语言全局保存。
Use the top-left profile button to switch among three independent run/history/
gameplay-settings profiles. Language is shared. Slot 1 keeps older save filenames.
更新前备份存档。新程序可读取旧存档；旧程序可能无法读取更新后写出的存档。
Back up the save before upgrading. This launcher reads older saves; an older
launcher may reject a save subsequently written by this version.
计算器上的局面和历史文件会自动选择较小的无损存储方式；旧文件无需手工转换。
Run and history files automatically use the smaller lossless storage form.
Existing raw files are still read directly; no manual conversion is needed.
计算器剩余空间较少时，USB 替换会临时占用新旧两份资源。升级前先删除旧的
nSpire2.common.tns、nSpire2.eng.tns、nSpire2.zhs.tns，再传入配套的新资源与程序。
只删除这三个资源文件；保留所有存档、备份、历史、设置和档位文件。
With little free calculator space, USB replacement can require both copies of
a resource temporarily. Before upgrading, remove the old nSpire2.common.tns,
nSpire2.eng.tns and nSpire2.zhs.tns, then transfer the matching new set and launcher.
Delete only those three resource files. Keep all run saves, backups, history,
settings and profile selector files.
旧版 nSpire2.nsp.tns 完整资源包无需与新的公共/语言包同时保留。
The old complete nSpire2.nsp.tns pack is unnecessary alongside split resources.

操作 / Controls
计算器用 8/2/4/6 移动，Enter / 5 选择，E 结束回合，Space 按住查看选牌界面下方的战场，Esc 返回/暂停。
计算器 Q 牌组，A 抽牌堆，D 弃牌堆，F 消耗牌堆，M 地图。
电脑默认鼠标与键盘：方向键移动，Enter 选择，E 结束回合，D 牌组，A 抽牌堆，S 弃牌堆，X 消耗牌堆。
在设置中可选择纯键盘操作并分别改键。I 详情，C 百科，V 状态，P 药水，R 遗物，O 读取。
常规设置中的“长按确认”默认关闭；开启后，结束回合需按住超过半秒，提前松开或移出会取消。
键盘选中手牌后 Enter 进入指向，左右选择目标，再按 Enter 出牌；Esc 取消。
Calculator: 8/2/4/6 navigate; Enter/5 selects; E ends the turn;
hold Space to peek beneath a combat card selection; Esc returns/pauses.
Q deck; A draw, D discard, F exhaust; M map.
Desktop mouse-and-keyboard defaults: arrows navigate; Enter selects; E ends
the turn; D deck; A draw, S discard, X exhaust; M map; Space peeks.
Settings has separate remappable mouse/keyboard and keyboard-only banks.
Long Press Confirmations in General is off by default. When enabled, hold End
turn for more than half a second; releasing early or moving away cancels it.
I details; C compendium; V statuses; P potions; R relics; O loads.
Touchpad clicking/dragging is supported. Save through the pause menu.
With a hand card selected, Enter aims, Left/Right chooses a target, Enter plays;
Esc cancels aiming. Fast Mode persists independently of the run save.
离线 HTML 可直接用浏览器打开，在主菜单的设置里切换语言；暂停菜单保存、O 读取，存档保留在该浏览器中。
Open the companion offline HTML in a browser; change language in game Settings.
Save from the pause menu and load with O; saves stay in that browser.
No asset download or server connection is needed.

版本 / Build
参考游戏 / game reference: v0.111.0 / 41cef1ea
Resource format: NSP2/{info['resource_format']}; save format: v{info['save_version']}
Installed files: {sum(info['files'].values()):,} bytes before saves
Runtime source: https://github.com/maxmixalex/slay-the-nspire-2/tree/{info['runtime_checkpoint']}
Resource converter: https://github.com/maxmixalex/slay-the-nspire-2/tree/{info['resource_checkpoint']}
程序源代码许可与第三方声明见 COPYING、NOTICE.md、licenses/。
See COPYING, NOTICE.md and licenses/ for source and third-party licenses.
原版游戏资源的权利属于 Mega Crit 及相应权利人。
Original game content belongs to Mega Crit and the respective rights holders.
'''
    if info.get('debug_launcher'):
        text += '''
可选调试版 / Optional developer build
nSpire2.tns 为普通版，调试指令默认关闭。
额外的 nSpire2-debug.tns 使用同一文件夹内的资源，按 G 打开指令菜单。
Normal nSpire2.tns omits developer commands. The additional nSpire2-debug.tns
uses the same resource files. Press G for its command menu, directions to choose
an operation/variant, Enter to execute and Esc to close.
浏览器调试 HTML 按 F1；控制台也可调用 nSpire2Debug('kill all')。
The separate debug HTML uses F1; its browser console also exposes
nSpire2Debug('kill all') and nSpire2Debug('room shop').
可选秒杀、进入普通/精英/首领/事件/商店/休息/宝藏/地图、跳幕等指令。
Available commands include killing enemies, entering rooms/encounters/events,
changing acts and starting a temporary run. Commands themselves do not autosave.
调试版与普通版共享存档位置。调试指令本身不自动存档，手动保存会写入调试后的局面。
Both launchers share the save location. An explicit Save writes the modified run.
完整指令 / Full command syntax:
https://github.com/maxmixalex/slay-the-nspire-2/blob/'''+info['runtime_checkpoint']+'''/docs/debug-commands-0.17.md
'''
    if info.get('calculator_only'):
        start=text.index('离线 HTML 可直接')
        end=text.index('\n版本 / Build',start)
        text=text[:start]+'''UI11 可读性与语言 / Readability and languages
主要正文使用10px、标题12px或更大；长文可滚动或翻页。I 打开完整卡牌详情。
地图图例按 I 展开，再按 I 或 Esc 收起；教学长文用8/2滚动。
图鉴长名称使用双行显示，行动较多时可翻页；原有游戏规则与存档保持兼容。
Main body text uses 10px, with larger headings. Long text scrolls or pages.
I opens full card details or the map legend. Use 8/2 to scroll long tutorials.
补充语言包中的每个 nSpire2.<语言代码>.tns 都可单独复制到同一文件夹。
只安装需要的语言；无需把16种语言全部复制到计算器。主菜单设置会列出已安装语言。
Install individual nSpire2.<locale>.tns files from the optional language archive.
Settings lists installed languages. All sixteen are supported; install only those needed.
本版不包含WASM/浏览器产物；界面与运行性能在Firebird计算器模拟器中验证。
This release contains the calculator build. UI and performance validation uses Firebird.
\n'''+text[end:]
    if info['resource_format'] >= 40:
        start=text.index('操作 / Controls')
        end=text.index('\nUI11 可读性与语言',start) if info.get('calculator_only') else text.index('\n离线 HTML 可直接',start)
        text=text[:start]+'''操作 / Controls
触摸板四边按下用于方向导航，中心用于点击；纯键盘模式下中心键选择。
数字 1–9、0 直接选择第 1–10 张手牌。Enter 进入指向/确认，左右换目标，Esc 取消。
Tab 地图；Doc 牌组；Home/On 抽牌堆；Del 弃牌堆；Var 消耗牌堆。
Menu 结束回合；Shift 按住查看选牌界面下面的战场；Ctrl 在纯键盘模式下进入顶部栏。
I 详情/地图图例，C 百科，V 状态，P 药水，R 遗物，O 读取；Esc 返回或暂停。
设置中的两套键位可分别修改。只自动迁移未改动过的旧默认键位，保留自定义设置。
长按确认默认关闭；开启后按住 Menu 超过半秒，提前松开或移出会取消结束回合。
Touchpad edge presses navigate; the centre clicks, or selects in keyboard-only mode.
Digits 1–9 and 0 select hand slots 1–10. Enter aims/confirms; arrows choose a target;
Esc cancels. Tab map; Doc deck; Home/On draw pile; Del discard; Var exhaust.
Menu ends the turn. Hold Shift to peek below a combat selection. Ctrl focuses
the top bar in keyboard-only mode. Settings has two separately remappable banks.
Untouched old defaults migrate automatically; custom bindings remain unchanged.
I details/map legend; C compendium; V statuses; P potions; R relics; O load.
Use the pause menu to save. With Long Press Confirmations enabled, hold Menu
for more than half a second; releasing early cancels the turn-end action.

原版控制台 / Developer console
按 Flag（Enter 上方的输入法切换键）打开/关闭控制台，直接输入命令，Enter 执行。
Tab 补全命令和模型名；方向键编辑或查看历史；Esc 取消补全/关闭控制台。
输入 help 查看命令，help card 等查看语法；例如 gold 10、card STRIKE_IRONCLAD、room Shop。
命令直接修改当前游戏，但本身不覆盖上次存档；主动保存会写入修改后的局面。
The normal launcher includes the original command console. Flag toggles it;
type commands and press Enter. Tab completes names, arrows edit/browse history,
and Esc cancels completion or closes the console. Use help or help card for syntax.
Commands change the current game without autosaving over the previous checkpoint;
an explicit Save persists the changed run. Desktop/network-only services report
that they are unavailable on this offline calculator.
'''+text[end:]
        text=text.replace('UI11 可读性与语言 / Readability and languages',
                          'UI12 可读性与语言 / Readability and languages')
        text=text.replace('地图图例按 I 展开，再按 I 或 Esc 收起；教学长文用8/2滚动。',
                          '地图图例按 I 或右下角按钮展开，再按 I 或 Esc 收起；长文用触摸板上下键滚动。')
        text=text.replace('I opens full card details or the map legend. Use 8/2 to scroll long tutorials.',
                          'I opens full card details or the map legend. Use touchpad Up/Down to scroll long text.')
        text=text.replace('主要正文使用10px、标题12px或更大；长文可滚动或翻页。I 打开完整卡牌详情。',
                          '主要正文使用10px、标题12px或更大；长文可滚动或翻页。I 打开完整卡牌详情。\n'
                          '选中长描述手牌后，正文会停留片刻再逐行滚动，费用、名称和相邻卡牌保持原位。\n'
                          '商店首次展开时会显示三个加载圆点；此时可以按 Esc 返回，准备完成后库存展开。')
        text=text.replace('Main body text uses 10px, with larger headings. Long text scrolls or pages.',
                          'Main body text uses 10px, with larger headings. Long text scrolls or pages.\n'
                          'Selected hand cards scroll long descriptions after a pause, within their body box.\n'
                          'The first cold shop opening shows three progress dots; Esc returns to the room.\n'
                          'Inventory opens once its artwork and card text are ready.')
    if info.get('local_checkpoints'):
        text=text.replace('Runtime source: https://github.com/maxmixalex/slay-the-nspire-2/tree/'+info['runtime_checkpoint'],
                          'Runtime source checkpoint (local): '+info['runtime_checkpoint'])
        text=text.replace('Resource converter: https://github.com/maxmixalex/slay-the-nspire-2/tree/'+info['resource_checkpoint'],
                          'Resource converter checkpoint (local): '+info['resource_checkpoint']+
                          '\nRepository: https://github.com/maxmixalex/slay-the-nspire-2')
    return text


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
    p.add_argument('--stage', default='ui4', help='Release suffix, for example ui4')
    p.add_argument('--debug-launcher', type=Path, help='Optional separately compiled developer launcher')
    p.add_argument('--debug-wasm', type=Path, help='Optional separately compiled developer browser runtime')
    args = p.parse_args()
    if not re.fullmatch('[A-Za-z0-9][A-Za-z0-9_-]*', args.stage):
        p.error('Stage must contain only letters, digits, underscores or hyphens')
    for value in (args.runtime_checkpoint, args.resource_checkpoint):
        if not re.fullmatch('[0-9a-f]{40}', value):
            p.error('Checkpoints must be full Git commit IDs')
    paths = {'nSpire2.tns': args.launcher, 'nSpire2.common.tns': args.common,
             'nSpire2.eng.tns': args.eng, 'nSpire2.zhs.tns': args.zhs}
    if not args.calculator_only and not args.wasm:
        p.error('Provide --wasm or choose --calculator-only')
    if not args.calculator_only and bool(args.debug_launcher) != bool(args.debug_wasm):
        p.error('Provide both optional debug runtimes together')
    if args.calculator_only and args.debug_wasm:
        p.error('--calculator-only does not accept --debug-wasm')
    if args.debug_launcher:
        paths['nSpire2-debug.tns'] = args.debug_launcher
    for path in (*paths.values(), *([args.wasm] if args.wasm else []), *([args.debug_wasm] if args.debug_wasm else [])):
        if not path.is_file():
            p.error('Input file is missing: ' + str(path))
    args.output.mkdir(parents=True, exist_ok=True)
    version = resource_version(args.eng)
    if resource_version(args.zhs) != version:
        p.error('English and Chinese resources must use the same format version')
    info = dict(reference='v0.111.0 / 41cef1ea', resource_format=version, save_version=17,
                progress_version=9, settings_version=6, calculator_run_storage='NSZ1 or raw NSV1',
                calculator_history_storage='NSZ1 or raw NSH1-9',
                stage=args.stage,
                runtime_checkpoint=args.runtime_checkpoint,
                resource_checkpoint=args.resource_checkpoint,
                debug_launcher=bool(args.debug_launcher),
                calculator_only=args.calculator_only,local_checkpoints=args.local_checkpoints,
                files={name: path.stat().st_size for name, path in paths.items()})
    stem = f'Slay-the-Nspire-2-0.17-{args.stage}-bilingual'
    archive = args.output / (stem + '-calculator.zip')
    offline = args.output / (stem + '-offline.html')
    texts = {'README.txt': readme(info).encode('utf-8'),
             'BUILD-INFO.json': (json.dumps(info, ensure_ascii=False, indent=2) + '\n').encode('utf-8')}
    for name in ('COPYING', 'NOTICE.md', 'licenses/Go-BSD-3-Clause.txt',
                 'licenses/OkHSV-Godot-MIT.txt', 'licenses/Noto-Boot-OFL.txt','licenses/NotoEmoji-OFL.txt','licenses/NotoSansSymbols2-OFL.txt'):
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
    if not args.calculator_only:
        package_runtime(destination=offline, common_path=args.common, eng_path=args.eng,
                        zhs_path=args.zhs, wasm_path=args.wasm,
                        source_checkpoint=args.runtime_checkpoint)
    debug_offline = None
    if args.debug_wasm:
        debug_offline = args.output / (stem + '-debug-offline.html')
        package_runtime(destination=debug_offline, common_path=args.common, eng_path=args.eng,
                        zhs_path=args.zhs, wasm_path=args.debug_wasm,
                        source_checkpoint=args.runtime_checkpoint)
    optional=None
    if args.optional_locales:
        extra={path.name:path for path in sorted(args.optional_locales.glob('nSpire2.*.tns'))
               if path.name not in ('nSpire2.common.tns','nSpire2.eng.tns','nSpire2.zhs.tns')}
        if len(extra)!=14 or any(resource_version(path)!=version for path in extra.values()):
            raise ValueError('Expected the fourteen additional matching original locales')
        optional=args.output/f'Slay-the-Nspire-2-0.17-{args.stage}-optional-languages.zip'
        instructions=('只复制需要的语言包，与配套公共包和程序放在同一文件夹，在主菜单设置切换语言。\n'
                      'Copy only desired language files beside the matching launcher/common file. Select in main-menu Settings.\n'
                      'English and Simplified Chinese are included in the main calculator archive.\n'+
                      ''.join(name+'  '+str(path.stat().st_size)+' bytes\n' for name,path in extra.items())).encode()
        with zipfile.ZipFile(optional,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as out:
            out.writestr('README.txt',instructions)
            for name,path in extra.items():out.write(path,name)
        with zipfile.ZipFile(optional) as check:
            if set(check.namelist())!=set(extra)|{'README.txt'} or check.read('README.txt')!=instructions:
                raise ValueError('Optional locale archive members differ')
            for name,path in extra.items():
                if check.read(name)!=path.read_bytes():raise ValueError('Optional locale differs: '+name)
    print(json.dumps(dict(calculator=str(archive), calculator_bytes=archive.stat().st_size,
                          offline=str(offline) if not args.calculator_only else None,
                          offline_bytes=offline.stat().st_size if not args.calculator_only else 0,
                          optional_locales=str(optional) if optional else None,
                          debug_offline=str(debug_offline) if debug_offline else None,
                          installed_bytes=sum(info['files'].values()), members=len(paths) + len(texts))))


if __name__ == '__main__':
    main()
