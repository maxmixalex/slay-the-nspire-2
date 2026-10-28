#!/usr/bin/env python3
"""Asset-free preservation and localization checks for private NSP2/12 packs."""
from pathlib import Path
import struct
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from build_content import fnv
from build_characters import localized,retained_prefix,assert_retained
from content_pack14 import PackData,PROFILES

def synthetic_v11():
    ni=ng=1;ns=407;nc=17;nk=114;nu=185
    start=40+ni*16+ng*16+ns*8+nc*16+nk*16+nu*4
    image=bytes(82*60*2)
    images=struct.pack('<IHHII',start,82,60,0,len(image))
    glyphs=struct.pack('<IIHBBHH',65,start+len(image),10,1,1,1,0)
    texts=struct.pack('<II',start+len(image)+1,1)*ns
    clips=struct.pack('<IHHhhI',0,1,33,0,0,0)*nc
    cards=struct.pack('<4I',0,0,0,0)*nk
    body=images+glyphs+texts+clips+cards+bytes(nu*4)+image+b'\xffA\0'
    return struct.pack('<4s9I',b'NSP2',11,0x41cef1ea,len(body)+40,ni,ng,ns,nc,nk,fnv(body))+body

def expanded():
    pack=PackData(synthetic_v11());prefix=retained_prefix(pack)
    nc,ns,nu=PROFILES[12]
    pack.images.append((82,60,0,bytes([7])*82*60*2))
    pack.strings+=['new title','base {Damage=7}','upgrade {Damage=10}']*((ns-407)//3)
    pack.cards+=[(1,407+i*3,408+i*3,409+i*3) for i in range(nc-114)]
    pack.aux+=[1]*(nu-185)
    pack.glyphs[66,10]=(1,1,1,0,b'\x80')
    return pack,prefix

class CharacterPackTests(unittest.TestCase):
    def test_frozen_profiles_and_append_roundtrip(self):
        self.assertEqual(PROFILES[10],(68,269,105))
        self.assertEqual(PROFILES[11],(114,407,185))
        pack,prefix=expanded();data=pack.encode(12);read=PackData(data)
        self.assertEqual(read.version,12);self.assertEqual(data,read.encode(12))
        assert_retained(read,prefix)
        self.assertEqual(read.images[1],pack.images[1])
    def test_repeated_append_locates_complete_old_prefix(self):
        pack,prefix=expanded();again=retained_prefix(PackData(pack.encode(12)))
        self.assertEqual(prefix[:5],again[:5])
        for key,value in prefix[5].items():self.assertEqual(again[5][key],value)
    def test_preservation_rejects_changed_old_records(self):
        for field in ('images','cards','strings','aux','clips','glyphs'):
            pack,prefix=expanded()
            if field=='images':pack.images[0]=(82,60,0,bytes([1])*82*60*2)
            elif field=='cards':pack.cards[0]=(0,1,0,0)
            elif field=='strings':pack.strings[0]='changed'
            elif field=='aux':pack.aux[0]=1
            elif field=='clips':pack.clips=pack.clips[:-1]+b'\x01'
            else:pack.glyphs[65,10]=(1,1,1,0,b'\x00')
            with self.subTest(field=field),self.assertRaises(ValueError):assert_retained(pack,prefix)
    def test_profile_cannot_be_relabelled_or_downgraded(self):
        with self.assertRaises(ValueError):PackData(synthetic_v11()).encode(12)
        pack,_=expanded()
        with self.assertRaises(ValueError):pack.encode(11)
        for data,version in ((synthetic_v11(),12),(pack.encode(12),11)):
            wrong=bytearray(data);struct.pack_into('<I',wrong,4,version)
            with self.subTest(version=version),self.assertRaises(ValueError):PackData(wrong)
    def test_precise_values_nested_branches_and_live_variables(self):
        row={'key':'SYNTHETIC','variables':{'Damage':{'base':9,'upgraded':12},
             'MaxHp':{'base':3,'upgraded':4},'CalculationBase':{'base':6,'upgraded':8},
             'Repeat':{'base':1,'upgraded':1}},
             'dynamic_variables':{'CalculatedDamage':'runtime','CalculatedHits':'runtime'},
             'keywords':{'base':['Exhaust'],'upgraded':['Exhaust','Retain']}}
        text='Hit {Damage:diff()}, gain {MaxHp:diff()}. {IfUpgraded:show:plus {CalculatedDamage:diff()}|base}{InCombat: hits {CalculatedHits:diff()}|}'
        keywords={'EXHAUST.title':'Exhaust','RETAIN.title':'Retain'}
        base=localized(text,row,False,keywords);upgraded=localized(text,row,True,keywords)
        self.assertIn('Hit {Damage=9}, gain 3.',base);self.assertIn('base\x1e hits {CalculatedHits=1}\x1f',base)
        self.assertIn('Hit {Damage=12}, gain 4.',upgraded);self.assertIn('plus {CalculatedDamage=8}',upgraded)
        self.assertNotIn('Retain',base);self.assertIn('Retain',upgraded)
    def test_unknown_or_malformed_reference_expressions_fail(self):
        row={'key':'SYNTHETIC','variables':{},'dynamic_variables':{},'keywords':{'base':[],'upgraded':[]}}
        for text in ('{Unverified:diff()}','{Energy:unsupported()}','{IfUpgraded:show:a|b|c}',
                     '{InCombat:a|nonempty}','{IfUpgraded:show:unclosed','stray }',
                     '{:diff()}','{Skills:choose(2):one|many}','{Skills:choose(1):only}',
                     '{Cards:plural:one|two|three}'):
            with self.subTest(text=text),self.assertRaises(ValueError):localized(text,row,False,{})
    def test_reference_count_formats_keep_exact_values_and_context(self):
        row={'key':'SYNTHETIC','variables':{'Cards':{'base':1,'upgraded':2},
             'Skills':{'base':1,'upgraded':2},'ExtraDamage':{'base':2,'upgraded':1}},
             'dynamic_variables':{},'keywords':{'base':[],'upgraded':[]}}
        plural='{Cards:diff()} {Cards:plural:item|items}'
        choose='{Skills:choose(1):one|{:diff()}}'
        inverse='{ExtraDamage:inverseDiff()}'
        self.assertEqual(localized(plural,row,False,{}),'1 item')
        self.assertEqual(localized(plural,row,True,{}),'2 items')
        self.assertEqual(localized(choose,row,False,{}),'one')
        self.assertEqual(localized(choose,row,True,{}),'2')
        self.assertEqual(localized(inverse,row,False,{}),'2')
        self.assertEqual(localized(inverse,row,True,{}),'1')
        nested='{Skills:choose(1):{:inverseDiff()}|{IfUpgraded:show:{:diff()}|other}}'
        self.assertEqual(localized(nested,row,False,{}),'1')
        self.assertEqual(localized(nested,row,True,{}),'2')
        with self.assertRaises(ValueError):localized('{Damage:choose(1):one|many}',row,False,{})

if __name__=='__main__':unittest.main()
