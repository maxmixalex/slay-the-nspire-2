"""Lossless dense stream selection and its complete ARM acceptance contract."""
import copy
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as t
from tile_filter import filter_tiles,restore_tiles
from select_pixel_streams_ui22 import prepare_candidates,validate_measurement


class PixelStreamSelectionTest(unittest.TestCase):
    @staticmethod
    def group():
        raw = bytearray()
        for tile in range(9):
            raw.extend(bytes((tile%16+1,16)))
            raw.extend(b''.join(struct.pack('<H',(pixel*701+tile*31)&65535) for pixel in range(256)))
            raw.extend(bytes(0 if pixel%3 else (pixel*13+tile)&255 for pixel in range(256)))
        raw = bytes(raw); filtered = filter_tiles(raw,2)
        encoded = b'\x01'+(len(filtered)-1).to_bytes(2,'big')+filtered+b'\0'
        return raw,t.GROUP.pack(b'TGR2',1,7,len(raw),len(encoded),0,9,2)+encoded

    def test_candidates_keep_every_color_alpha_dimension_and_group_id(self):
        raw,original = self.group()
        with tempfile.TemporaryDirectory() as folder:
            actual,candidates = prepare_candidates(original,1024*1024,Path(folder))
            self.assertEqual(actual,raw); self.assertEqual(len(candidates),3)
            for payload,_profile in candidates:
                header=t.GROUP.unpack_from(payload)
                self.assertEqual((header[1],header[2],header[3],header[6]),(1,7,len(raw),9))
                decoded=t.decode_lzma2(payload[32:],len(raw),1024*1024)
                self.assertEqual(restore_tiles(decoded,header[7]) if header[7] else decoded,raw)
                self.assertLess(len(payload)+((len(original)-len(payload))&1),len(original))
            # Cache reuse is bound to decoded content, not a global group ID.
            relocated=bytearray(original);struct.pack_into('<I',relocated,8,19)
            _,again=prepare_candidates(bytes(relocated),1024*1024,Path(folder))
            self.assertEqual([x[0][32:] for x in candidates],[x[0][32:] for x in again])
            self.assertTrue(all(t.GROUP.unpack_from(x[0])[2]==19 for x in again))

    def test_corrupt_cached_stream_is_rejected(self):
        _,original=self.group()
        with tempfile.TemporaryDirectory() as folder:
            prepare_candidates(original,1024*1024,Path(folder))
            path=next(Path(folder).glob('encoded/*/mode0.bin'))
            value=path.read_bytes()+b'\0';path.write_bytes(value)
            meta=path.with_suffix('.json');record=json.loads(meta.read_text());record['sha256']=t.sha256(value)
            meta.write_text(json.dumps(record))
            with self.assertRaises(ValueError):prepare_candidates(original,1024*1024,Path(folder))

    def test_complete_cpu_gate_rejects_slower_and_foreign_evidence(self):
        key,raw_sha='01'*32,'02'*32
        def counters(stored,lzma,inverse):
            return dict(lzma_instructions=lzma,inverse_instructions=inverse,
                total_instructions=lzma+inverse,stored_bytes=stored,decoded_bytes=770,
                restored_sha256=raw_sha,exact_records_guards_aapcs=True)
        record=dict(schema=1,complete=True,key=key,before=counters(400,800,200),after=counters(300,850,100))
        self.assertTrue(validate_measurement(record,key,400,300,770,raw_sha))
        slower=copy.deepcopy(record);slower['after']=counters(300,950,100)
        self.assertFalse(validate_measurement(slower,key,400,300,770,raw_sha))
        for field,value in [('complete',False),('key','03'*32)]:
            changed=copy.deepcopy(record);changed[field]=value
            with self.assertRaises(ValueError):validate_measurement(changed,key,400,300,770,raw_sha)
        for field,value in [('restored_sha256','04'*32),('stored_bytes',301),
                            ('total_instructions',0),('exact_records_guards_aapcs',False)]:
            changed=copy.deepcopy(record);changed['after'][field]=value
            with self.assertRaises(ValueError):validate_measurement(changed,key,400,300,770,raw_sha)


if __name__ == '__main__':unittest.main()
