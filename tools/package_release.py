#!/usr/bin/env python3
"""Package matching private calculator resources, with an optional browser player."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
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
    profile = info.get('animation_profile', 'animated')
    animation = ('本包仅保留必要人物/怪物动作姿态，保留原有动作时长；Ancient房间人物及背景效果保持入场姿态，卡牌和UI仍按各自时钟运行。启动器自动识别，无需切换设置。\n'
                 'This edition keeps essential actor poses and original action durations. Ancient NPC/background effects hold their entry pose; card and UI clocks continue. The launcher detects it automatically.\n') \
                if profile == 'no-animations' else \
                ('本包包含完整采样人物/怪物动画。\nThis edition includes the complete sampled actor animations.\n')
    shared = ('data/nSpire2.dat.tns         公共资源索引 / artwork index\n' +
              ''.join('data/'+name+'\n' for name in PART_FILES)) if info.get('multipart_artwork') else \
             'data/nSpire2.dat.tns         公共图片与动画 / shared artwork and animation\n'
    sources = ''
    for key, label in (('runtime', 'Runtime'), ('resource', 'Resource converter')):
        actual = info[key + '_checkpoint']
        published = info.get(key + '_source_ref',
                             None if info.get('local_checkpoints') else actual)
        if published is None:
            sources += f'{label} source checkpoint (local): {actual}\n'
        elif published == actual:
            sources += f'{label} source: https://github.com/maxmixalex/slay-the-nspire-2/tree/{published}\n'
        else:
            sources += (f'Actual {label.lower()} build checkpoint (local): {actual}\n'
                        f'Published equivalent {label.lower()} source: '
                        f'https://github.com/maxmixalex/slay-the-nspire-2/tree/{published}\n')
    if not info.get('runtime_source_ref') and not info.get('resource_source_ref') and info.get('local_checkpoints'):
        sources += 'These source checkpoints have not been published to GitHub.\n'
    documentation = info.get('documentation_checkpoint')
    if documentation:
        sources += ("Documentation: https://github.com/maxmixalex/slay-the-nspire-2/blob/"
                    f"{documentation}/docs/ui23-retained-differences.md\n")
    ledger_zh = '上列独立文档链接' if documentation else '上列运行源代码中的 docs/ui23-retained-differences.md'
    ledger_en = ('the separately linked documentation above' if documentation else
                 'docs/ui23-retained-differences.md in the runtime source above')
    validation = ''
    ui24 = bool(re.match(r'ui(?:2[4-9]|[3-9][0-9])(?:$|[-_])', info.get('stage', ''), re.IGNORECASE))
    timeline = ('时间线 / Timeline | 57个原版节点的获得、揭示、分数与解锁链已接入正常界面和存档；原版Early Access占位仍不可用。旧档保留兼容开放，不伪造历史日期。57 source Epochs, obtain/reveal state, score and unlock chains are connected to normal UI and saves. Source Early Access placeholders remain unavailable. Legacy profiles retain compatibility access without invented dates.\n' if ui24 else '')
    missing = '音频和多人游戏当前未实现。Audio and multiplayer are not implemented.' if ui24 else '音频、多人游戏和Timeline当前未实现。Audio, multiplayer and Timeline are not implemented.'
    seed = ('种子输入64个UTF-8字节；更长ASCII文本可经配套转换器编码后，用自定义模式的导入种子码功能完整导入。Seed text accepts 64 UTF-8 bytes; the matching converter can encode complete longer ASCII input for the explicit Import seed code mode.' if ui24 else '种子64个UTF-8字节。Seed text accepts 64 UTF-8 bytes.')
    compatibility = ('''升级兼容 / Save compatibility
保留整个 state 文件夹。第一档局面为 state/nSpire2.sav.tns，进度为 state/nSpire2.progress.tns；第二/三档分别使用 nSpire2.sav.p2.tns、nSpire2.progress.p2.tns 与 p3 文件。设置、历史、语言、备份也在 state。
UI24可读取旧版v3–v17局面、NSHA及更早进度；保存后写v18/NSHB，旧启动器不能保证读取新格式。需要回退时，使用升级前完整备份的 state 与其配套旧程序/资源。
Preserve the entire state folder. Profile one uses state/nSpire2.sav.tns and state/nSpire2.progress.tns. Profiles two/three insert .p2/.p3 before .tns, such as nSpire2.sav.p2.tns and nSpire2.progress.p2.tns. Settings, history, language and backups also live there.
UI24 reads v3–v17 runs and NSHA and older progress. New saves use v18/NSHB; older launchers may not read them. To roll back, restore the complete pre-upgrade state backup together with its matching old launcher/resources.

时间线 / Timeline
从主菜单打开时间线，选择节点按 Enter 查看，再按 Enter 揭示已获得的节点。新档按原版解锁；旧档兼容保留已开放内容，不补造过去的获得时间。
Open Timeline from the title menu. Select an Epoch and press Enter to inspect, then Enter again to reveal an obtained Epoch. Fresh profiles follow source unlocks; legacy profiles retain existing content access without invented obtain dates.

长 ASCII 种子 / Longer ASCII seeds
电脑端运行 nspire2-convert --encode-seed-file seed.txt，或 nspire2-convert --encode-seed "完整ASCII种子"。PowerShell 使用 ./nspire2-convert.exe。
计算器自定义模式中打开种子输入框，按上下切换到“导入种子码”，输入输出的 SEED64- 加16位十六进制码，再确认。不要在普通文本模式输入此码；普通文本始终按原版文本规则哈希。
On the computer, run nspire2-convert --encode-seed-file seed.txt, or nspire2-convert --encode-seed "complete ASCII seed". Use ./nspire2-convert.exe in PowerShell.
In Custom Run, open the seed field and use Up/Down to select Import seed code. Enter the generated SEED64- plus16 hexadecimal digits and confirm. Ordinary text mode always hashes the text using the original rules; it does not auto-detect import codes.
''' if ui24 else '')
    if info.get('stage', '').startswith(('ui23', 'ui24')):
        validation = f'''
验证范围 / Verification scope
本包没有计算器实机测试记录，不能据此给出实机帧率或实机峰值内存。
文件与资源校验摘要位于 data/BUILD-INFO.json；详细校验记录单独保存，不占计算器安装空间。
原尺寸画质、原生运行与模拟器证据的范围及保留差异，见{ledger_zh}。
No physical-calculator test is recorded for this package. Calculator FPS and physical peak RAM remain unmeasured.
File and resource validation summaries are in data/BUILD-INFO.json; detailed records are kept separately.
See {ledger_en} for visual, native/emulator evidence and remaining differences.

具体边界 / Remaining boundaries
项目 / Item | 本包状态 / Status
七种附着与历史 / Afflictions and history | 七种附着使用连续材质与粒子；32场、96层、4096条通知为可归档工作窗口。旧版已经覆盖的数据不能恢复。Continuous materials/particles and disk-backed history pages are implemented; previously erased history cannot be recovered.
画面与动画 / Rendering | 320×240 RGB565软件绘制，保留已说明的采样/噪声近似；未证明任意Spine混轨或未知原GPU随机历史等价。Software rendering retains documented sampling/noise bounds; arbitrary Spine mixing and unrecorded GPU random history are not established.
服务与输入 / Services and input | 未提供云存档、在线排行榜、桌面时区/DST数据库或计算器IME。Cloud saves, hosted leaderboards, desktop timezone/DST database and calculator IME are not supplied.
容量 / Capacity | 地图绘画2048点，达到容量时保留已有笔画并显示提示；这不是地图生成节点上限。{seed} 历史归档受存储空间、序号和文件偏移范围限制。Map annotations retain existing strokes and report a full 2048-point buffer; this does not limit generated map nodes. History is bounded by disk space, serial and file-offset ranges.
新增提示语言 / New diagnostic text | 历史/归档错误与时间线状态提示提供简体中文和英语，其余语言显示英语；节点原文使用对应原版语言，缺失键回退英语。Port-authored history/archive errors and Timeline status labels use Simplified Chinese or English; original Epoch text uses the selected original locale, with English fallback for missing source keys.
{timeline}未实现功能 / Unimplemented features | {missing} 本包不声称桌面功能全等。This package does not claim full desktop feature equivalence.
实测范围 / Measurement | 已记录的模拟器结果不是实机帧率，未测实机峰值内存。Recorded emulator results are not physical-handheld FPS or measured physical peak RAM.
'''
    return f"""Slay the Nspire 2 — nSpire {info['version']} (StS II v0.111.0)

安装 / Installation
首次安装：将 nSpire2.tns 与 data、state 文件夹一起复制到计算器上的游戏目录，打开 nSpire2.tns。
Copy nSpire2.tns and the data/state folders into the game's calculator directory.
Open nSpire2.tns. A color TI-Nspire with Ndless is required.
{animation}

nSpire2.tns                  程序 / launcher
{shared.rstrip()}
data/nSpire2.eng.dat.tns     English
data/nSpire2.zhs.dat.tns     简体中文
state/                      局面、历史、设置、语言和档位 / saves and settings

只安装需要的语言文件。设置中可切换已安装语言；语言选择全局保存。
公共资源索引、全部用途包与语言文件必须来自同一次配套打包/转换，并采用相同动画档，不能按用途删减。
升级前退出游戏，保留已有 state 文件夹及其中的档案，不要用包内的空 state 覆盖它。
删除游戏目录中旧的整个 data 文件夹，然后传入本包的完整 data 文件夹并替换 nSpire2.tns。
可删除新 data 中不需要的语言文件，但必须保留配套的新索引和全部用途包。
之前安装的可选语言也必须换成本轮对应文件，不能混用旧版。
自行转换的资源必须使用同一次转换生成的可选语言，不能混用预制语言ZIP，即使版本和动画档相同。
这同样适用于旧分包版升级，可避免传输时同时暂存旧大文件和新资源。
等全部配套文件传输完成后再启动。可选语言只传需要的几种，不必安装全部十六种。
首次启动按系统语言选择；对应语言缺失或无法加载时尝试英语，再选择首个可用语言。
主菜单左上角可切换三个档案，各自保存局面、历史与游戏设置。
程序会创建缺失的 state 文件夹。所有保存、临时文件、备份和诊断文件均写入 state。
Install only the desired language files in data. Settings lists installed languages.
Install the artwork index, every concern file and locales from the same matching packaging/conversion and animation profile. Every concern file is required.
Exit the game before updating; keep existing files in state.
Do not overwrite the existing state folder with the archive's empty state folder.
Delete the entire old data folder first, then copy this archive's matching data folder
and replace nSpire2.tns. You may omit unwanted locale files, but keep the
matching index and every concern file.
Replace previously installed optional languages with this release's matching files too.
For freshly converted resources, use optional locales from that same conversion;
do not mix them with the prebuilt locale ZIP, even when version/profile labels match.
This also applies to updates from an older multipart installation and avoids
temporarily storing both old large files and their replacements during transfer.
Start only after the complete matching set is present.
Install only the additional languages you need; all sixteen are not required.
First launch follows the system language, then English, then the first usable language.
Missing or unusable language resources fall back through the same sequence. Startup errors are English.
The top-left profile button selects one of three independent run/history/settings profiles.
Language is shared. The launcher creates state if necessary; all mutable files stay there.
{compatibility}

操作 / Controls
触摸板四边导航，中心点击；数字 1–9、0 选择第 1–10 张手牌。
Enter 指向/确认；左右选择目标；Esc 返回/暂停。
Tab 地图；Doc 牌组；Home/On 抽牌堆；Del 弃牌堆；Var 消耗牌堆。
Menu 结束回合；自定义/每日挑战表单中用 Menu 开始；Shift 按住查看选牌界面下方的战场；Ctrl 在纯键盘模式下聚焦顶部栏。
I 详情/地图图例，C 百科，V 状态，P 药水，R 遗物，O 读取。
暂停菜单可保存或退出；两套键位可在设置中分别修改。
制作人员名单中按住空格键加速滚动。
Touchpad edges navigate; its centre clicks. Digits 1–9 and 0 select hand slots 1–10.
Enter aims/confirms; arrows choose targets; Esc returns or pauses.
Tab map; Doc deck; Home/On draw pile; Del discard; Var exhaust; Menu ends the turn or starts the Custom/Daily form.
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
{validation.rstrip()}
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
    p.add_argument('--runtime-source-ref', help='Verified published equivalent runtime source; actual build checkpoint is retained (calculator packages only)')
    p.add_argument('--resource-source-ref', help='Verified published equivalent converter source; actual producer checkpoint is retained (calculator packages only)')
    p.add_argument('--documentation-ref', help='Optional independent published documentation commit; does not relabel runtime or resources')
    p.add_argument('--stage', default='ui22', help='Release suffix; validated against its complete content profile')
    args = p.parse_args()
    if not re.fullmatch('[A-Za-z0-9][A-Za-z0-9_-]*', args.stage):
        p.error('Stage must contain only letters, digits, underscores or hyphens')
    for value in (args.runtime_checkpoint, args.resource_checkpoint,
                  *(value for value in (args.documentation_ref, args.runtime_source_ref, args.resource_source_ref) if value)):
        if not re.fullmatch('[0-9a-f]{40}', value):
            p.error('Checkpoints must be full Git commit IDs')
    if args.runtime_source_ref or args.resource_source_ref:
        if args.local_checkpoints:
            p.error('Published source refs cannot be combined with --local-checkpoints')
        if not args.calculator_only:
            p.error('Separate build/source identities currently require --calculator-only')
    runtime_source_ref = args.runtime_source_ref or (None if args.local_checkpoints else args.runtime_checkpoint)
    resource_source_ref = args.resource_source_ref or (None if args.local_checkpoints else args.resource_checkpoint)
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
    animation_profile = None
    for code,path in (('eng',args.eng),('zhs',args.zhs)):
        logical = reconstruct(common_bytes,path.read_bytes(),code)
        profile = ('no-animations' if len(logical) >= 40 and
                   struct.unpack_from('<I', logical, 4)[0] >= 46 and
                   logical[36:40] == b'NAF1' else 'animated')
        if code == 'eng':
            content_validation = validate_release_content(logical, args.stage)
            animation_profile = profile
        elif profile != animation_profile:
            p.error('English and Chinese animation profiles differ')
        del logical
    base_version = (ROOT/'VERSION').read_text().strip().split('-',1)[0]
    release_version = f'{base_version}-{args.stage}'
    stage_number = re.match(r'ui([0-9]+)(?:$|[-_])', args.stage, re.IGNORECASE)
    timeline_schema = bool(stage_number and int(stage_number[1]) >= 24)
    info = dict(version=release_version, base_version=base_version, reference='v0.111.0 / 41cef1ea', resource_format=version, save_version=18 if timeline_schema else 17,
                progress_version=11 if timeline_schema else 10, progress_formats=[f'NSH{i}' for i in range(1,10)]+(['NSHA','NSHB'] if timeline_schema else ['NSHA']),
                settings_version=6, calculator_run_storage='NSZ1 or raw NSV1',
                calculator_history_storage='NSZ1 or raw NSH1-9/NSHA'+('/NSHB' if timeline_schema else ''),
                calculator_history_archive=dict(file='state/nSpire2.history.tns',container='NHA1',
                    record='NHR1',page='NHP2',readable_pages=['NHP1','NHP2']),
                stage=args.stage,
                animation_profile=animation_profile,
                runtime_checkpoint=args.runtime_checkpoint,
                resource_checkpoint=args.resource_checkpoint,
                runtime_source_ref=runtime_source_ref, resource_source_ref=resource_source_ref,
                developer_console=True, installation_layout="data/state",multipart_artwork=multipart,
                locale_storage='NSL3' if args.eng.read_bytes()[:4]==b'NSL3' else 'NSL1/NSL2',
                calculator_only=args.calculator_only,
                local_checkpoints=runtime_source_ref!=args.runtime_checkpoint or resource_source_ref!=args.resource_checkpoint,
                files={name: path.stat().st_size for name, path in paths.items()})
    if args.documentation_ref:
        info['documentation_checkpoint'] = args.documentation_ref
    validation_bytes = None
    if content_validation is not None:
        validation_bytes = (json.dumps(content_validation, ensure_ascii=False, sort_keys=True, indent=2)+'\n').encode('utf-8')
        info['required_content_validation'] = dict(complete=content_validation['complete'],
            auxiliary_count=content_validation.get('auxiliary_count'),
            checked_frame_images=content_validation.get('checked_frame_images'),
            framed_tables=len(content_validation.get('framed_tables', [])),
            detailed_record_sha256=hashlib.sha256(validation_bytes).hexdigest(),
            detailed_record_bytes=len(validation_bytes), details_packaged=False)
    info['program_resource_sha256'] = {name: hashlib.sha256(path.read_bytes()).hexdigest()
                                      for name, path in paths.items()}
    if args.stage.startswith(('ui23','ui24')):
        info['physical_device_tested'] = False
        info['calculator_fps_measured'] = False
        info['physical_peak_ram_measured'] = False
    stem = f'Slay-the-Nspire-2-{release_version}-bilingual'
    archive = args.output / (stem + '-calculator.zip')
    offline = args.output / (stem + '-offline.html')
    if validation_bytes is not None:
        (args.output/(stem+'-validation.json')).write_bytes(validation_bytes)
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
        optional=args.output/f'Slay-the-Nspire-2-{release_version}-optional-languages.zip'
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
