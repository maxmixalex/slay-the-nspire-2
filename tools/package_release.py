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
    version=info.get('base_version',info['version'])
    edition='精简动画 / reduced animation' if info['animation_profile']=='no-animations' else '完整动画 / full animation'
    return f'''Slay the Nspire 2 {version} — {edition}
Reference: Slay the Spire 2 v0.111.0 / 41cef1ea

安装 / Install
full.zip 与 reduced.zip 任选其一，两个主包均含英语和简体中文。
将 nSpire2.tns 和完整 data 文件夹放在计算器同一目录，使用 Ndless 打开 nSpire2.tns。
Choose either full.zip or reduced.zip. Both include English and Simplified Chinese.
Copy nSpire2.tns and the complete data folder into one directory on a color TI-Nspire with Ndless, then open nSpire2.tns.

升级前退出游戏并备份整个 state 文件夹。删除旧 data，复制新 data 并替换 nSpire2.tns；保留原 state，不用空文件夹覆盖存档。
语言、资源索引及所有用途文件须来自同一配套版本和动画档；可删除不需要的语言，但不能删除用途包。
Exit and back up the entire state folder before updating. Replace the old data folder and nSpire2.tns; preserve your existing state folder.
Keep the index, all artwork files and languages from the same matching conversion and animation edition.
You may remove unwanted language files. Keep every artwork file.

额外语言 / Additional languages
languages.zip 的 animated/data 配 full.zip，no-animations/data 配 reduced.zip。
仅复制需要的 nSpire2.<language>.dat.tns 到游戏 data；在设置中选择语言。自行转换的数据只能使用同次转换的语言。
Use animated/data with full.zip or no-animations/data with reduced.zip. Copy only the desired locale files into the game's data folder, then select a language in Settings.
Independently converted data needs its own matching locale files.

本版 / This version
已移除时间线界面和内容门槛；角色按各自前置角色的游玩进度解锁。新档教程从正常开局进入。
Timeline screens and content gates are removed. Character availability follows independent prerequisite-character play progress. New profiles enter the tutorial through normal play.
精简版保留人物/怪物必要姿态及动作时长；Ancient人物/背景保持入场姿态，卡牌和界面时钟继续运行。
The reduced edition keeps essential actor poses and action durations. Ancient NPC/background effects hold their entry pose; card and UI clocks continue.

操作 / Controls
方向键导航，触摸板移动指针并点击，Enter确认，Esc返回或暂停。数字1–9、0选择手牌，Menu结束回合。
Tab地图，Doc牌组，Home/On抽牌堆，Del弃牌堆，Var消耗牌堆。Shift按住查看选择界面后的战场。
设置中可调整触摸轻点、指针速度及两套按键。暂停菜单可保存或退出。
Use arrows to navigate, the touchpad to move/click, Enter to confirm and Esc to go back or pause. Digits 1–9 and 0 select cards; Menu ends the turn.
Tab opens the map, Doc the deck, Home/On the draw pile, Del the discard pile and Var the exhaust pile. Hold Shift to peek behind a selection.
Settings includes Tap to Click, pointer speed and two remappable key banks. The pause menu saves or exits.

存档与转换 / Saves and conversion
保留整个 state；三个档案各自保存局面、进度和设置，语言全局共享。旧存档可读取，回退时使用升级前整套备份。
Preserve all of state. The three profiles have separate runs, progress and settings; language is shared. Older saves remain readable. Restore the complete pre-upgrade backup when rolling back.
原版目录中运行配套转换器，默认生成中英完整数据；--no-animations 生成精简版，--languages all 生成全部16种语言。
Run the matching converter in your owned original game directory. Its default produces full bilingual data; --no-animations selects reduced animation and --languages all includes all 16 original locales.

说明 / Notes
320×240软件绘制及采样动画。音频、多人和在线服务未实现；未进行实体计算器测试。
320×240 software rendering with sampled animation. Audio, multiplayer and online services are not implemented. Physical-calculator performance has not been measured.
Source: https://github.com/maxmixalex/slay-the-nspire-2
Program and resources: {sum(info['files'].values()):,} bytes before instructions, licenses and saves.
详细文件记录见 BUILD-INFO.json；许可见 licenses。原版素材属于 Mega Crit 及各权利人。
See BUILD-INFO.json for file details and licenses for notices. Original game assets belong to Mega Crit and their respective rights holders.
'''


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
    p.add_argument('--resource-source-ref', help='Published converter source reference; actual producer checkpoint is retained (calculator packages only)')
    p.add_argument('--resource-source-relation', choices=('equivalent_tree','published_implementation_reference'), default='equivalent_tree', help='Whether the published converter reference is an exact whole-tree equivalent')
    p.add_argument('--resource-provenance',type=Path,help='Private resource lineage to include in BUILD-INFO.json')
    p.add_argument('--documentation-ref', help='Optional independent published documentation commit; does not relabel runtime or resources')
    p.add_argument('--stage', default='ui26', help='Release suffix; validated against its complete content profile')
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
    info = dict(version=release_version, base_version=base_version, reference='v0.111.0 / 41cef1ea', resource_format=version, save_version=18,
                progress_version=12,
                progress_formats=[f'NSH{i}' for i in range(1,10)]+['NSHA','NSHB','NSHC'],
                settings_version=7, calculator_run_storage='NSZ1 or raw NSV1',
                calculator_history_storage='NSZ1 or raw NSH1-9/NSHA/NSHB/NSHC',
                calculator_history_archive=dict(file='state/nSpire2.history.tns',container='NHA1',
                    record='NHR1',page='NHP2',readable_pages=['NHP1','NHP2']),
                stage=args.stage,
                animation_profile=animation_profile,
                runtime_checkpoint=args.runtime_checkpoint,
                resource_checkpoint=args.resource_checkpoint,
                runtime_source_ref=runtime_source_ref, resource_source_ref=resource_source_ref,
                resource_source_relation=args.resource_source_relation,
                developer_console=True, installation_layout="data/state",multipart_artwork=multipart,
                locale_storage='NSL3' if args.eng.read_bytes()[:4]==b'NSL3' else 'NSL1/NSL2',
                calculator_only=args.calculator_only,
                local_checkpoints=runtime_source_ref!=args.runtime_checkpoint or resource_source_ref!=args.resource_checkpoint,
                files={name: path.stat().st_size for name, path in paths.items()})
    if args.documentation_ref:
        info['documentation_checkpoint'] = args.documentation_ref
    if args.resource_provenance:
        lineage=json.loads(args.resource_provenance.read_text(encoding='utf-8'))
        if not isinstance(lineage,dict) or not isinstance(lineage.get('full_cold_conversion'),bool):
            p.error('Resource provenance must explicitly state whether a full cold conversion was executed')
        stage_source=lineage.get('new_stage_source',{})
        if (stage_source.get('actual_checkpoint')!=args.resource_checkpoint or
                stage_source.get('published_checkpoint')!=resource_source_ref):
            p.error('Resource provenance does not match the actual and published producer checkpoints')
        info['resource_provenance']=lineage
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
