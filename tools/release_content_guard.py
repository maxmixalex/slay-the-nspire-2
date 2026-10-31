"""Validate content required by a named release before writing player artifacts.

A format version describes a decoder, not which optional UI extensions exist.
UI20 requires actual checked framed tables and reward images; a legacy catalog
converted to the same wire format must not be published under that release name.
"""
from __future__ import annotations

import re
import struct

import content_tiles as tiles
from repack_retained import auxiliary_count

BASE_AUX = 5409
REQUIRED_AUX = 5422
CLIPS = (105, 123, 124, 125, 548, 706, 707, 1263, 1626)
CLOCKS = ((17, 78, 10177), (35, 83, 16149), (17, 82, 6647),
          (90, 83, 6144), (45, 83, 13987), (38, 81, 18428),
          (40, 82, 5678), (12, 83, 15124), (31, 81, 18264))
MAGIC = 0x4652
UI22_BASE_AUX = 5422
UI22_CLIPS = 1783
UI22_ACTOR_AUX = 7206
UI22_REQUIRED_AUX = 7212
# Pinned UI22 coverage for reference 41cef1ea. Zero-valued clip units were
# resolved through the model registration when selecting this source pass.
UI22_CLIP_RANGES = ((0,3),(11,15),(17,20),(22,25),(27,30),(32,36),(43,51),(60,66),
    (105,113),(119,129),(452,462),(474,478),(495,499),(545,559),(579,584),
    (603,663),(680,698),(704,715),(731,736),(755,779),(786,792),(1075,1114),
    (1120,1125),(1143,1150),(1163,1181),(1207,1229),(1235,1249),(1259,1269),
    (1310,1314),(1338,1349),(1621,1629),(1710,1741),(1753,1764))
UI22_EXPECTED = frozenset(i for first,last in UI22_CLIP_RANGES for i in range(first,last+1)) | {UI22_CLIPS}
UI22_DEATH_CLOCKS = ((71,33),(57,33),(68,33),(126,33),(71,33))*2+((68,33),(151,33))
UI22_SELECTION_COUNTS = (64,140,160,80,96)


def validate_ui22(source, image):
    tiles.require(len(source.aux) >= UI22_ACTOR_AUX+5 and len(source.clips) == UI22_CLIPS and
                  len(source.images) >= 58612,
                  'UI22 requires the complete native actor catalog and five selection markers (7211 auxiliaries)')
    entries=source.aux[UI22_BASE_AUX:UI22_ACTOR_AUX]
    registered={i for i,value in enumerate(entries) if value}
    tiles.require(registered==UI22_EXPECTED,'UI22 native actor registry is incomplete or includes an unsupported route')
    table_ids={entries[i] for i in registered}
    tiles.require(len(table_ids)==427,'UI22 native actor parameter identities must be distinct')
    tiles.require(not any(entries[1383:1615]),'UI22 must preserve the dedicated Vantom mesh route')
    forbidden=table_ids|set(source.aux[BASE_AUX:REQUIRED_AUX]);tables=[];checked_parts=set()
    for index in sorted(registered):
        width,height,fmt,raw=image(entries[index])
        tiles.require(fmt==0 and len(raw)==width*height*2 and len(raw)>=20,
                      'UI22 actor parameters must be exact word images')
        words=struct.unpack('<'+'H'*(len(raw)//2),raw)
        magic,version,frames,ms=words[:4]
        units=words[4]|words[5]<<16;reference=words[6]|words[7]<<16;used,stride=words[8:10]
        tiles.require(magic==MAGIC and version==1 and stride==3 and 0<frames<=256 and
                      0<ms<=1000 and 0<units<1000000 and 0<reference<1000000 and
                      10+2*frames<=used<=len(words),'Invalid UI22 actor table header')
        if index<UI22_CLIPS:
            first,count,clock,_px,_py,old_units=source.clips[index]
            tiles.require((frames,ms)==(count,clock),'UI22 actor frame clock differs from the stable clip')
            if 1753<=index<=1764:
                tiles.require((frames,ms)==UI22_DEATH_CLOCKS[index-1753],
                              'UI22 terminal death/particle sample schedule changed')
            if old_units:
                tiles.require(reference==old_units,'UI22 actor original registration changed')
        else:
            first=None
            tiles.require((frames,ms,reference)==(3,56,4254),'UI22 Osty dead_loop clock or registration changed')
        next_word=10+2*frames;parts=0
        for frame in range(frames):
            start,layers=words[10+2*frame:12+2*frame]
            tiles.require(start==next_word and 0<layers<=16 and start+3*layers<=used,
                          'UI22 actor piece range is invalid')
            left=top=32768;right=bottom=-32768
            for part in range(layers):
                picture,x,y=words[start+3*part:start+3*part+3]
                x=x-65536 if x>=32768 else x;y=y-65536 if y>=32768 else y
                tiles.require(0<=picture<len(source.images) and picture not in forbidden,
                              'UI22 actor piece refers outside its pixel catalog')
                if first is not None and part==0:
                    tiles.require(picture==first+frame,'UI22 actor primary frame identity changed')
                else:
                    tiles.require(picture>=58099,'UI22 actor overflow must use its appended source slot')
                _offset,pw,ph,pfmt,_size=source.images[picture]
                tiles.require(0<pw<=320 and 0<ph<=240 and pfmt in (0,1,2,3,4,5,6,7),
                              'UI22 actor piece dimensions are outside the bounded renderer')
                left=min(left,x);top=min(top,y);right=max(right,x+pw);bottom=max(bottom,y+ph)
                checked_parts.add(picture)
            tiles.require(right-left<=2560 and bottom-top<=1920,'UI22 actor frame union exceeds the verified bound')
            next_word+=layers*3;parts+=layers
        tiles.require(next_word==used and not any(words[used:]),'UI22 actor parameter padding or extent is invalid')
        tables.append(dict(clip=index if index<UI22_CLIPS else -2,frames=frames,frame_ms=ms,parts=parts))
    selection=[]
    for character,count in enumerate(UI22_SELECTION_COUNTS):
        first,frames,ms,*_=source.clips[1705+character]
        tiles.require((frames,ms)==(count,83) and source.aux[UI22_ACTOR_AUX+character]==first,
                      'UI22 selection marker or source clock is missing')
        for frame in range(frames):
            width,height,fmt,raw=image(first+frame)
            tiles.require((width,height)==(320,240) and fmt in (0,4),'UI22 selection must use full viewport exact copy patches')
            if fmt==0:
                tiles.require(len(raw)==320*240*2,'UI22 opaque selection frame length differs');continue
            cursor=0
            for row in range(240):
                tiles.require(cursor+2<=len(raw),'UI22 selection sparse row is truncated')
                spans=struct.unpack_from('<H',raw,cursor)[0];cursor+=2
                tiles.require(frame!=0 or spans==1,'UI22 selection base is not opaque')
                previous=0
                for span in range(spans):
                    tiles.require(cursor+5<=len(raw),'UI22 selection span is truncated')
                    x,length,mode=struct.unpack_from('<HHB',raw,cursor);cursor+=5
                    tiles.require(mode==0 and length>0 and x>=previous and x+length<=320 and
                                  (frame!=0 or (x==0 and length==320)),
                                  'UI22 selection patch is not an exact opaque copy')
                    cursor+=2*length;previous=x+length
                    tiles.require(cursor<=len(raw),'UI22 selection copy pixels are truncated')
            tiles.require(cursor==len(raw),'UI22 selection patch has trailing bytes')
        selection.append(dict(character=character,first_image=first,frames=frames,frame_ms=ms))
    return dict(complete=True,registered_tables=len(tables),registered_frames=sum(t['frames'] for t in tables),
                registered_pieces=len(checked_parts),source_clip_ids_pinned=True,vantom_generic_entries=0,
                selection=selection,selection_frames=sum(r['frames'] for r in selection),tables=tables)


def validate_release_content(data, stage):
    """Validate publishable content; UI22 and newer always require NSP2/46."""
    return _validate_content(data, stage, retirement_intermediate=False)


def validate_retirement_intermediate(data):
    """Check the exact NSP2/45 retirement boundary before implicit geometry.

    This is not a release profile. It shares every artwork, actor, selection,
    retired-root and unsupported-geometry check with the final UI22 guard;
    only its explicitly pinned intermediate wire/catalog schema differs.
    """
    result = _validate_content(data, 'ui22', retirement_intermediate=True)
    result.update(validation_scope='retirement-before-implicit-geometry',
                  content_version=45, final_release_ready=False,
                  required_next_content_version=46)
    return result


def _validate_content(data, stage, *, retirement_intermediate):
    match = re.match(r'ui([0-9]+)(?:$|[-_])', stage, re.IGNORECASE)
    if not match or int(match[1]) < 20:
        return None
    count = auxiliary_count(data)
    if int(match[1])>=22:
        tiles.require(count>=UI22_REQUIRED_AUX,
                      'UI22 content is missing: complete native actors, selection and guarded legacy retirement require 7212 auxiliaries')
    tiles.require(count >= REQUIRED_AUX,
                  'UI20 content is missing: expected framed actions, Osty dead_loop and reward icons (5422 auxiliaries)')
    source = tiles.PackView(data, count, versions=tuple(range(44, tiles.MAXIMUM_CONTENT_VERSION + 1)))
    from no_animations import reduced, validate_profile
    reduced_profile = reduced(data)
    if 'no-animations' in stage.lower() or 'noanimations' in stage.lower():
        tiles.require(reduced_profile, 'The no-animation release requires its NAF1 resource profile')
    if retirement_intermediate:
        tiles.require(source.version==45 and count==7212 and len(source.images)==58613,
                      'Retirement intermediate requires exact NSP2/45 with 7212 auxiliaries and 58613 images')
        tiles.require(not reduced_profile and struct.unpack_from('<I',data,36)[0]==0,
                      'Retirement intermediate must precede animation profile selection')
    elif int(match[1])>=22:
        tiles.require(source.version>=46,
                      'UI22 final content requires NSP2/46 implicit geometry support')
    tiles.require(len(source.clips) >= 1783 and len(source.images) >= 58099,
                  'UI20 complete image/clip catalog is missing')
    table_ids = source.aux[BASE_AUX:BASE_AUX + 10]
    reward_ids = source.aux[BASE_AUX + 10:REQUIRED_AUX]
    tiles.require(len(set(table_ids + reward_ids)) == 13, 'UI20 auxiliary entries must be distinct')
    forbidden = set(table_ids + reward_ids)
    reader = None
    checked_pixels = set()

    def image(image_id):
        nonlocal reader
        tiles.require(0 <= image_id < len(source.images), 'UI20 layer image is outside catalog')
        if source.images[image_id][3] == 6:
            if reader is None:
                reader = tiles.TileReader(source)
            if int(match[1])>=22:
                # This index is also needed immediately by image_raw, so the
                # release decision shares its cache rather than adding a scan.
                _base,local,at,size=tiles.DESCRIPTOR.unpack_from(source.data,source.images[image_id][0])
                group,starts=reader.group(reader.tile_groups+local)
                tiles.require(starts.get(at)==size,'UI22 image names an incomplete index record')
                ids=struct.unpack_from('<I',group,at+4)[0]
                tiles.require(struct.unpack_from('<H',group,at+tiles.FRAME.size+ids)[0]!=0xfffc,
                              'UI22 release rejects experimental selection bitmap geometry (FFFC)')
            return reader.image_raw(image_id)
        return source.image_raw(image_id)

    tables = []
    for row, image_id in enumerate(table_ids):
        width, height, fmt, raw = image(image_id)
        tiles.require(fmt == 0 and len(raw) == width * height * 2 and len(raw) >= 20,
                      'UI20 framed registry must be an exact word image')
        words = struct.unpack('<' + 'H' * (len(raw) // 2), raw)
        magic, version, frames, frame_ms = words[:4]
        units, reference = words[4] | words[5] << 16, words[6] | words[7] << 16
        used, stride = words[8:10]
        tiles.require(magic == MAGIC and version == 1 and stride == 3 and
                      0 < frames <= 256 and 0 < frame_ms <= 1000 and
                      0 < units < 1000000 and 0 < reference < 1000000 and
                      10 + 2 * frames <= used <= len(words), 'Invalid UI20 framed registry header')
        if row < len(CLIPS):
            clip = CLIPS[row]
            first, expected_frames, expected_ms, _x, _y, expected_reference = source.clips[clip]
            tiles.require((expected_frames, expected_ms, expected_reference) == CLOCKS[row],
                          'UI20 pinned enemy clip metadata differs')
            tiles.require((frames, frame_ms, units, reference) ==
                          (expected_frames, expected_ms, 5376, expected_reference),
                          'UI20 enemy frame clock or native registration differs')
        else:
            clip = -2
            tiles.require((frames, frame_ms, units, reference) == (3, 56, 4254, 4254),
                          'UI20 Osty dead_loop is missing or has the wrong clock')
        next_word = 10 + 2 * frames
        parts = 0
        for frame in range(frames):
            start, layers = words[10 + 2 * frame:12 + 2 * frame]
            tiles.require(start == next_word and 0 < layers <= 16 and start + 3 * layers <= used,
                          'UI20 framed layer range is invalid')
            if row < len(CLIPS):
                tiles.require(words[start] == first + frame,
                              'UI20 enemy frame does not retain its original image identity')
            for layer in range(layers):
                part = words[start + 3 * layer]
                tiles.require(part not in forbidden, 'UI20 frame refers to a registry or reward icon')
                if part not in checked_pixels:
                    image(part)  # Exercise the actual retained/tiled decoder, not just dimensions.
                    checked_pixels.add(part)
            next_word += 3 * layers
            parts += layers
        tiles.require(next_word == used and not any(words[used:]),
                      'UI20 framed registry has unclaimed or nonzero padding words')
        tables.append(dict(clip=clip, auxiliary=BASE_AUX + row, frames=frames,
                           frame_ms=frame_ms, parts=parts))
    rewards = []
    for name, image_id in zip(('rare', 'uncommon', 'special'), reward_ids):
        width, height, fmt, raw = image(image_id)
        tiles.require((width, height) == (23, 23), 'UI20 reward icon must use its native 23px size')
        rewards.append(dict(name=name, image=image_id, decoded_sha256=tiles.sha256(raw)))
    tiles.require(len({row['decoded_sha256'] for row in rewards}) == 3,
                  'UI20 reward icons must contain distinct artwork')
    result=dict(complete=True, auxiliary_count=count, framed_tables=tables,
                checked_frame_images=len(checked_pixels), reward_icons=rewards)
    if int(match[1])>=22:
        result['ui22']=validate_ui22(source,image)
        from retired_assets_ui22 import validate_retired_profile
        result['ui22']['retirement']=validate_retired_profile(source,image)
    if reduced_profile:
        result['animation_profile'] = validate_profile(source,image)
    return result
