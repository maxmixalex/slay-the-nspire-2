"""A named UI release must contain its actual content, not just a newer codec."""
from pathlib import Path
import struct
import sys
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import content_tiles as tiles
import release_content_guard as guard
import retired_assets_ui22 as retired


def fixture(auxiliary=5422):
    # Public synthetic artwork. Preserve production directory dimensions and
    # clocks so a shape-only/format-only gate cannot pass these negative cases.
    ni, ng, ns, nc, nk = (58613 if auxiliary >= 7212 else 58612 if auxiliary >= 7211 else 58099), 1, 1, 1783, 1
    glyph_at = 40 + ni * 16
    string_at = glyph_at + 16
    clip_at = string_at + 8
    aux_at = clip_at + nc * 16 + 16
    base = aux_at + auxiliary * 4
    out = bytearray(base + 64)
    out[base:base + 4] = b'TIL1'

    def append(raw):
        if len(out) & 1:
            out.append(0)
        offset = len(out)
        out.extend(raw)
        return offset

    pixel = append(b'\x11\x22')
    image = tiles.IMAGE.pack(pixel, 1, 1, 0, 2)
    out[40:glyph_at] = image * ni
    glyph = append(b'\xff')
    tiles.GLYPH.pack_into(out, glyph_at, 65, glyph, 8, 1, 1, 1, 0)
    string = append(b'eng\0')
    struct.pack_into('<II', out, string_at, string, 3)
    clip = tiles.CLIP.pack(0, 1, 100, 0, 0, 5376)
    out[clip_at:clip_at + nc * 16] = clip * nc
    for clip_id, (frames, ms, reference) in zip(guard.CLIPS, guard.CLOCKS):
        tiles.CLIP.pack_into(out, clip_at + clip_id * 16, 0, frames, ms, 0, 0, reference)
    table_offsets = []
    old_images=58612 if auxiliary>=7211 else ni
    table_ids = list(range(old_images - 13, old_images - 3))
    for row, image_id in enumerate(table_ids):
        if row < 9:
            frames, ms, reference = guard.CLOCKS[row]
            units = 5376
        else:
            frames, ms, units, reference = 3, 56, 4254, 4254
        words = [guard.MAGIC, 1, frames, ms, units, 0, reference, 0, 0, 3]
        words.extend([0] * (2 * frames))
        for frame in range(frames):
            words[10 + 2 * frame:12 + 2 * frame] = [len(words), 1]
            words.extend([frame if row < 9 else 0, 0, 0])
        words[8] = len(words)
        width = min(320, len(words))
        height = (len(words) + width - 1) // width
        words.extend([0] * (width * height - len(words)))
        raw = struct.pack('<' + 'H' * len(words), *words)
        offset = append(raw)
        table_offsets.append(offset)
        tiles.IMAGE.pack_into(out, 40 + image_id * 16, offset, width, height, 0, len(raw))
        if 5409 + row < auxiliary:
            struct.pack_into('<I', out, aux_at + (5409 + row) * 4, image_id)
    for i, image_id in enumerate(range(old_images - 3, old_images)):
        raw = struct.pack('<H', 0x1100 + i) * (23 * 23)
        offset = append(raw)
        tiles.IMAGE.pack_into(out, 40 + image_id * 16, offset, 23, 23, 0, len(raw))
        if 5419 + i < auxiliary:
            struct.pack_into('<I', out, aux_at + (5419 + i) * 4, image_id)
    native_offsets={}
    if auxiliary>=7211:
        for index,(frames,ms) in enumerate(guard.UI22_DEATH_CLOCKS):
            tiles.CLIP.pack_into(out,clip_at+(1753+index)*16,0,frames,ms,0,0,5376)
        for ordinal,index in enumerate(sorted(guard.UI22_EXPECTED)):
            image_id=58099+ordinal
            if index<guard.UI22_CLIPS:
                first,frames,ms,_px,_py,reference=tiles.CLIP.unpack_from(out,clip_at+index*16)
                units=min(reference,5376)
            else:frames,ms,units,reference=3,56,2688,4254
            words=[guard.MAGIC,1,frames,ms,units,0,reference,0,0,3]+[0]*(2*frames)
            for frame in range(frames):
                words[10+2*frame:12+2*frame]=[len(words),1]
                words.extend([first+frame if index<guard.UI22_CLIPS else 58596+frame,0,0])
            words[8]=len(words);width=min(320,len(words));height=(len(words)+width-1)//width
            words.extend([0]*(width*height-len(words)));raw=struct.pack('<'+'H'*len(words),*words);offset=append(raw)
            tiles.IMAGE.pack_into(out,40+image_id*16,offset,width,height,0,len(raw))
            struct.pack_into('<I',out,aux_at+(guard.UI22_BASE_AUX+index)*4,image_id);native_offsets[index]=offset
        opaque=append(b'\x11\x22'*(320*240));empty=append(b'\0\0'*240)
        first=40000 if auxiliary>=7212 else 20000
        for character,count in enumerate(guard.UI22_SELECTION_COUNTS):
            tiles.CLIP.pack_into(out,clip_at+(1705+character)*16,first,count,83,0,0,0)
            struct.pack_into('<I',out,aux_at+(guard.UI22_ACTOR_AUX+character)*4,first)
            for frame in range(count):
                tiles.IMAGE.pack_into(out,40+(first+frame)*16,empty if frame else opaque,320,240,4 if frame else 0,480 if frame else 320*240*2)
            first+=count
    if auxiliary>=7212:
        # A complete public synthetic profile exercises the exact retirement
        # whitelist and both typed reference grammars without private pixels.
        empty=append(b'\0\0')
        for row in retired.PLAN['retired_clips']:
            tiles.CLIP.pack_into(out,clip_at+row['id']*16,*row['tuple'])
            first,count=row['tuple'][:2]
            for image_id in range(first,first+count):
                tiles.IMAGE.pack_into(out,40+image_id*16,empty,1,1,4,2)
        for row in retired.PLAN['merchant_clips']:
            tiles.CLIP.pack_into(out,clip_at+row['id']*16,*row['tuple'])
            width,height=row['size'];raw=b'\x11\x22'*(width*height);at=append(raw)
            first,count=row['tuple'][:2]
            for image_id in range(first,first+count):
                tiles.IMAGE.pack_into(out,40+image_id*16,at,width,height,0,len(raw))
        for row in retired.PLAN['merchant_auxiliary']:
            width,height=row['size'];raw=b'\x11\x22'*(width*height);at=append(raw)
            tiles.IMAGE.pack_into(out,40+row['image']*16,at,width,height,0,len(raw))
            struct.pack_into('<I',out,aux_at+row['id']*4,row['image'])
        words=[0x5654,3,1,33,1024,0,0,0,7,12,24,0,17,1,0,0,24,
               0,0,0,0,0,65535,65535]
        raw=struct.pack('<24H',*words);at=append(raw)
        for i in range(116):
            image_id=50000+i;tiles.IMAGE.pack_into(out,40+image_id*16,at,24,1,0,len(raw))
            struct.pack_into('<I',out,aux_at+(5288+i)*4,image_id)
        marker_at=append(retired.marker_bytes())
        tiles.IMAGE.pack_into(out,40+retired.MARKER_IMAGE*16,marker_at,8,1,0,16)
        struct.pack_into('<I',out,aux_at+retired.BASE_AUX*4,retired.MARKER_IMAGE)
    tiles.HEADER.pack_into(out, 0, b'NSP2', 46 if auxiliary>=7212 else 45, tiles.REFERENCE, len(out), ni, ng, ns, nc, nk, 0)
    return out, dict(aux=aux_at, tables=table_offsets, table_ids=table_ids,native=native_offsets,clips=clip_at,
                     marker=marker_at if auxiliary>=7212 else None)


class ReleaseContentGuardTest(unittest.TestCase):
    def test_complete_extension_and_older_stage_policy(self):
        data, _ = fixture()
        report = guard.validate_release_content(data, 'ui20')
        self.assertTrue(report['complete'])
        self.assertEqual(report['auxiliary_count'], 5422)
        self.assertEqual(sum(row['frames'] for row in report['framed_tables']), 328)
        self.assertEqual(report['framed_tables'][-1]['frame_ms'], 56)
        self.assertEqual(len(report['reward_icons']), 3)
        self.assertIsNone(guard.validate_release_content(b'not an NSP file', 'ui19'))
        self.assertIsNone(guard.validate_release_content(b'public fixtures', 'public-web'))

    def test_legacy_catalog_is_rejected_even_after_format_upgrade(self):
        data, _ = fixture(auxiliary=5409)
        for stage in ('ui20', 'UI20', 'ui20-preview', 'ui21', 'ui021_test'):
            with self.subTest(stage=stage), self.assertRaisesRegex(ValueError, 'UI20 content is missing'):
                guard.validate_release_content(data, stage)

    def test_header_layer_clock_and_reward_corruption_is_rejected(self):
        original, at = fixture()
        cases = (
            ('magic', at['tables'][0], 0, 'registry header'),
            ('frame clock', at['tables'][0] + 6, 1, 'frame clock'),
            ('empty layers', at['tables'][0] + 22, 0, 'layer range'),
            ('layer offset', at['tables'][0] + 20, 65535, 'layer range'),
            ('image identity', at['tables'][0] + (10 + 2 * 17) * 2, 58086, 'image identity'),
            ('Osty clock', at['tables'][9] + 6, 55, 'Osty dead_loop'),
        )
        for label, offset, value, error in cases:
            data = bytearray(original)
            struct.pack_into('<H', data, offset, value)
            with self.subTest(label=label), self.assertRaisesRegex(ValueError, error):
                guard.validate_release_content(data, 'ui20')
        data = bytearray(original)
        # A directory-sized copied icon must not masquerade as the gold set.
        data[40 + 58097 * 16:40 + 58098 * 16] = data[40 + 58096 * 16:40 + 58097 * 16]
        with self.assertRaisesRegex(ValueError, 'distinct artwork'):
            guard.validate_release_content(data, 'ui20')
        data = bytearray(original)
        struct.pack_into('<I', data, at['aux'] + 5418 * 4, at['table_ids'][0])
        with self.assertRaisesRegex(ValueError, 'distinct'):
            guard.validate_release_content(data, 'ui20')


    def test_ui22_requires_actor_and_selection_completion(self):
        for auxiliary in (5422,7206,7211):
            data,_=fixture(auxiliary)
            with self.subTest(auxiliary=auxiliary),self.assertRaisesRegex(ValueError,'UI22 content is missing'):
                guard.validate_release_content(data,'ui22')
        data,_=fixture(7212);result=guard.validate_release_content(data,'UI22-release')['ui22']
        self.assertTrue(result['complete']);self.assertEqual(result['registered_tables'],427)
        self.assertEqual(result['selection_frames'],540);self.assertEqual(result['vantom_generic_entries'],0)
        struct.pack_into('<I',data,4,45)
        with self.assertRaisesRegex(ValueError,'NSP2/46'):
            guard.validate_release_content(data,'ui22')

    def test_retirement_intermediate_is_complete_but_never_publishable(self):
        data,_=fixture(7212)
        with self.assertRaisesRegex(ValueError,'exact NSP2/45'):
            guard.validate_retirement_intermediate(data)
        struct.pack_into('<I',data,4,45)
        result=guard.validate_retirement_intermediate(data)
        self.assertTrue(result['complete'])
        self.assertTrue(result['ui22']['retirement']['complete'])
        self.assertEqual(result['ui22']['registered_tables'],427)
        self.assertEqual(result['ui22']['selection_frames'],540)
        self.assertFalse(result['final_release_ready'])
        self.assertEqual(result['required_next_content_version'],46)
        for stage in ('ui22','ui23','ui23-retirement-intermediate'):
            with self.subTest(stage=stage),self.assertRaisesRegex(ValueError,'NSP2/46'):
                guard.validate_release_content(data,stage)
        other,_=fixture(7213);struct.pack_into('<I',other,4,45)
        with self.assertRaisesRegex(ValueError,'exact NSP2/45'):
            guard.validate_retirement_intermediate(other)

    def test_retirement_intermediate_keeps_artwork_and_liveness_guards(self):
        original,at=fixture(7212);struct.pack_into('<I',original,4,45)
        cases=(
            ('framed clock',at['tables'][0]+6,'H',1,'frame clock'),
            ('native actor',at['aux']+guard.UI22_BASE_AUX*4,'I',0,'registry'),
            ('death clock',at['native'][1757]+6,'H',31,'frame clock'),
            ('selection',at['aux']+guard.UI22_ACTOR_AUX*4,'I',40001,'selection marker'),
            ('marker',at['marker']+8,'I',retired.PLAN['plan_tag']^1,'plan marker'),
            ('active root',at['aux']+123*4,'I',321,'direct clip/card/auxiliary root'),
            ('typed root',at['tables'][9]+16*2,'H',321,'typed actor/mesh/mask root'),
            ('unretired pixels',40+321*16+8,'I',0,'empty sparse'),
            ('premature profile',36,'I',0x3146414e,'profile selection'),
        )
        for label,offset,fmt,value,error in cases:
            data=bytearray(original);struct.pack_into('<'+fmt,data,offset,value)
            with self.subTest(label=label),self.assertRaisesRegex(ValueError,error):
                guard.validate_retirement_intermediate(data)

    def test_ui22_missing_actor_and_wrong_registry_routes_fail(self):
        original,at=fixture(7212)
        for index,value in ((1757,0),(1383,58099),(0,58100)):
            data=bytearray(original);struct.pack_into('<I',data,at['aux']+(guard.UI22_BASE_AUX+index)*4,value)
            with self.subTest(index=index),self.assertRaisesRegex(ValueError,'registry|identities'):
                guard.validate_release_content(data,'ui22')

    def test_ui22_rejects_experimental_selection_bitmap_before_pixels(self):
        data,at=fixture(7212)
        first=struct.unpack_from('<I',data,at['clips']+1705*tiles.CLIP.size)[0]
        # The full profile above uses public literal pixels. Replace only one
        # selection patch with a checked-record reader to isolate the release
        # policy; decoding this experimental image must never be requested.
        geometry=bytearray(240*(2+16*5));geometry[:2]=b'\xfc\xff'
        geometry[2:40]=b'\xff'*37+b'\x0f'
        for y in range(240):
            for x in range(0,320,20):
                for pixel in range(x,x+8):geometry[40+y*40+pixel//8]|=1<<(pixel%8)
        record=tiles.FRAME.pack(240*(2+16*5+16*8*2),1200,len(geometry))+bytes(1200)+geometry
        descriptor=len(data);data.extend(tiles.DESCRIPTOR.pack(0,0,0,len(record)))
        tiles.IMAGE.pack_into(data,40+(first+1)*tiles.IMAGE.size,descriptor,320,240,6,16)
        struct.pack_into('<I',data,12,len(data))
        reader=mock.Mock(tile_groups=0)
        reader.group.return_value=(record,{0:len(record)})
        for version,validate in ((46,lambda raw:guard.validate_release_content(raw,'ui22')),
                                 (45,guard.validate_retirement_intermediate)):
            struct.pack_into('<I',data,4,version)
            with self.subTest(version=version),mock.patch.object(tiles,'TileReader',return_value=reader),self.assertRaisesRegex(ValueError,'experimental selection bitmap'):
                validate(data)
        reader.image_raw.assert_not_called()

    def test_ui22_clock_bounds_and_selection_contract_fail(self):
        original,at=fixture(7212)
        data=bytearray(original);struct.pack_into('<H',data,at['native'][1757]+6,31)
        with self.assertRaisesRegex(ValueError,'frame clock'):guard.validate_release_content(data,'ui22')
        data=bytearray(original);struct.pack_into('<H',data,at['native'][1757]+6,31)
        struct.pack_into('<H',data,at['clips']+1757*16+6,31)
        with self.assertRaisesRegex(ValueError,'terminal death'):guard.validate_release_content(data,'ui22')
        data=bytearray(original);struct.pack_into('<H',data,40+100*16+4,321)
        with self.assertRaisesRegex(ValueError,'dimensions'):guard.validate_release_content(data,'ui22')
        data=bytearray(original);struct.pack_into('<I',data,at['aux']+guard.UI22_ACTOR_AUX*4,40001)
        with self.assertRaisesRegex(ValueError,'selection marker'):guard.validate_release_content(data,'ui22')
        data=bytearray(original);data[40+40000*16:40+40001*16]=data[40+40001*16:40+40002*16]
        with self.assertRaisesRegex(ValueError,'base is not opaque'):guard.validate_release_content(data,'ui22')
        data=bytearray(original);raw=struct.pack('<HHHB',1,0,1,1)+b'\0\0\x7f'+b'\0\0'*239
        offset=len(data);data.extend(raw);tiles.IMAGE.pack_into(data,40+40001*16,offset,320,240,4,len(raw))
        struct.pack_into('<I',data,12,len(data))
        with self.assertRaisesRegex(ValueError,'exact opaque copy'):guard.validate_release_content(data,'ui22')


if __name__ == '__main__':
    unittest.main()
