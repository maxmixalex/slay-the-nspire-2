#!/usr/bin/env python3
"""Synthetic, asset-free checks for the new pack directory conversion."""
from pathlib import Path
import struct
import json
import re
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from build_events import PackData
from build_content import fnv

def synthetic():
    ni=ng=1;ns=228;nc=17;nk=68;nu=95
    start=40+ni*16+ng*16+ns*8+nc*16+nk*16+nu*4
    images=struct.pack('<IHHII',start,1,1,0,2)
    glyphs=struct.pack('<IIHBBHH',65,start+2,10,1,1,1,0)
    texts=struct.pack('<II',start+3,1)*ns
    clips=struct.pack('<IHHhhI',0,1,33,0,0,0)*nc
    cards=struct.pack('<4I',0,0,0,0)*nk
    body=images+glyphs+texts+clips+cards+bytes(nu*4)+b'\0\0\xffA\0'
    return bytearray(struct.pack('<4s9I',b'NSP2',9,0x41cef1ea,len(body)+40,ni,ng,ns,nc,nk,fnv(body))+body)

def rehash(data):
    struct.pack_into('<I',data,36,fnv(data[40:]));return bytes(data)

class PackTests(unittest.TestCase):
    def test_browser_keeps_reference_and_isolated_save_keys(self):
        root=Path(__file__).resolve().parents[1]
        lock=json.loads((root/'config/reference-release.json').read_text())
        page=(root/'platform/web/player.html').read_text()
        for language in ('eng', 'zhs'):
            locale=json.loads((root/f'config/web-locale-{language}.json').read_text())
            self.assertEqual(set(re.findall(r'v0\.\d+\.0',locale['about'])),{lock['version']})
            self.assertIn(lock['commit'],locale['about'])
        storage=(root/'platform/web/profile_storage.js').read_text()
        host=(root/'platform/web/main.cpp').read_text()
        self.assertIn('getUint32(4,true)',page)
        self.assertIn('profileStorage.writeRun(saveProfile,encode(bytes),version,supportedSaveVersion())',page)
        self.assertIn('profileStorage.readRun(saveProfile,supportedSaveVersion())',page)
        self.assertIn('unsigned save_version(){return spire::SaveVersion;}',host)
        self.assertIn('latestSaveVersion=17',storage)
        self.assertIn("key('nSpire2-save-v'+version,slot)",storage)
    def test_roundtrip_preserves_records(self):
        p=PackData(synthetic());legacy=(p.images[:],p.glyphs.copy(),p.clips,p.cards[:]);p.strings+=['new']*41;p.aux+=[0]*10
        output=p.encode();q=PackData(output)
        self.assertEqual(output,q.encode());self.assertEqual(legacy,(q.images,q.glyphs,q.clips,q.cards))
    def test_rejects_truncation_without_payload_integrity_scans(self):
        base=synthetic()
        for size in (0,1,39,40,len(base)-1):
            with self.subTest(size=size),self.assertRaises(ValueError):PackData(base[:size])
        base[-2]^=1
        self.assertEqual(PackData(base).strings[0], '@')
        struct.pack_into('<I',base,36,0)
        self.assertEqual(PackData(base).strings[0], '@')
    def test_rejects_out_of_bounds_directories(self):
        for off,value in ((40,0),(44,321),(48,6),(16,8193),(24,229)):
            data=synthetic();struct.pack_into('<I',data,off,value)
            with self.subTest(off=off),self.assertRaises(ValueError):PackData(rehash(data))
    def test_rejects_invalid_references(self):
        data=synthetic();cards_at=40+16+16+228*8+17*16;struct.pack_into('<I',data,cards_at,2)
        with self.assertRaises(ValueError):PackData(rehash(data))
        data=synthetic();struct.pack_into('<I',data,cards_at+4,228)
        with self.assertRaises(ValueError):PackData(rehash(data))
    def test_requires_complete_output_profile(self):
        with self.assertRaises(ValueError):PackData(synthetic()).encode()

if __name__=='__main__':unittest.main()
