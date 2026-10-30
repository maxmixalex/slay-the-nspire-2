"""Small synthetic v17 relocation and language-boundary checks (no game data)."""
from pathlib import Path
import sys
from tempfile import TemporaryDirectory
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import content_tiles as tiles
from append_native_ui import append_skins
from locale_cards import bind_template,model_rows
from localize_content import repack,format_catalog,legacy_single_player,glyph_alphabet
from locale_profile import validate_generated,bindings_and_keys,direct_bindings,inline_bindings,runtime_symbols,FRONTEND_TEXTS,UI_BASE
from test_content_tiles import fixture


class LocalePack(unittest.TestCase):
    def test_rebuilt_text_and_fonts_preserve_every_graphic(self):
        with TemporaryDirectory() as temporary:
            root=Path(temporary);source=root/'source';tiled=root/'tiled'
            source.write_bytes(fixture(extra=2,shared_retained=True))
            tiles.repack(source,tiled,root/'work',auxiliary_count=1,first_clip=1,tiles_per_group=1,workers=1)
            baseline,_=append_skins(tiled.read_bytes(),1,[(1,1,0,b'\x12\x34')])
            old=tiles.PackView(baseline,2,versions=(16,));reader=tiles.TileReader(old)
            glyphs=[(63,8,2,1,2,0,b'\x55\x99')]
            skin=(1,1,1,b'\x56\x78\xff')
            results=[]
            for texts in (['eng','Synthetic English'],['zhs','Synthetic alternative']):
                output,receipt=repack(baseline,2,texts,glyphs,[skin])
                new=tiles.PackView(output,3,versions=(17,));new_reader=tiles.TileReader(new)
                for i,image in enumerate(old.images):
                    expected=reader.image_raw(i) if image[3]==6 else old.image_raw(i)
                    actual=new_reader.image_raw(i) if image[3]==6 else new.image_raw(i)
                    self.assertEqual(actual,expected)
                self.assertEqual(new.cards,old.cards);self.assertEqual(new.clips,old.clips)
                self.assertEqual(new.aux[:2],old.aux)
                self.assertEqual(new.image_raw(len(old.images)),skin)
                self.assertEqual(new.glyphs[0][0],63)
                for text,(at,length) in zip(texts,new.strings):
                    self.assertEqual(output[at:at+length+1],text.encode()+b'\0')
                results.append(receipt['original_graphics_sha256'])
            self.assertEqual(results[0],results[1])
            for bad in (['eng','bad\0text'],['eng','x'*tiles.STRING_LIMIT],['eng',None]):
                with self.assertRaises(ValueError):repack(baseline,2,bad,glyphs)

    def test_original_grammar_keeps_explicit_catalog_values(self):
        row=dict(key='SYNTHETIC',variables={'Count':dict(base=1,upgraded=2)})
        template='{IfUpgraded:show:A|B} {Count:plural:{Count:diff()} item|{Count:diff()} items}'
        self.assertEqual(bind_template(template,row,True),'{IfUpgraded:show:A|B} {Count=2:plural:{Count=2:diff()} item|{Count=2:diff()} items}')
        with self.assertRaises(ValueError):bind_template('{Unknown:diff()}',row,False)
        self.assertEqual([row['id'] for row in model_rows()],list(range(558)))

    def test_potion_plural_and_original_resource_icons(self):
        tables={'static_hover_tips':{'ENERGY_COUNT.title':'ENERGY_UNIT','STAR_COUNT.title':'STAR_UNIT'}}
        text='{Count:plural:{Count:diff()} item|{Count:diff()} items}: {N:energyIcons()} / {N:starIcons()}'
        # Pinned EnergyIconsFormatter repeats 1..3 images and otherwise uses
        # number+image; StarIconsFormatter repeats images. PUA values are the
        # port's image tokens, never translated hover-title substitutes.
        self.assertEqual(format_catalog(text,dict(Count=1,N=2),tables,'eng'),'1 item: \ue001\ue001 / \ue000\ue000')
        self.assertEqual(format_catalog(text,dict(Count=3,N=2),tables,'eng'),'3 items: \ue001\ue001 / \ue000\ue000')
        self.assertEqual(format_catalog('0{energyPrefix:energyIcons(1)}',{},tables,'eng'),'0\ue001')
        self.assertEqual(format_catalog('{N:energyIcons()}',dict(N=4),tables,'eng'),'4\ue001')
        with self.assertRaises(ValueError):format_catalog('{Missing:diff()}',{},tables,'eng')

    def test_every_port_literal_has_a_reviewed_binding(self):
        validate_generated();rows,keys=bindings_and_keys()
        self.assertLessEqual(UI_BASE+len(rows),FRONTEND_TEXTS[0][0]);self.assertLessEqual(len(keys),1057)
        self.assertEqual(FRONTEND_TEXTS,((8103,'settings_ui.TAB_GENERAL'),(8104,'settings_ui.TAB_INPUT')))
        self.assertTrue(any(row.get('reference')=='static_hover_tips.STAR_COUNT.title' for row in rows))

    def test_japanese_calendar_periods_keep_their_direct_culture_binding(self):
        # These are ja culture facts, not Chinese fallback prose. Putting them
        # in the legacy translated table would move existing private-pack slots.
        direct={row['source']:row for row in direct_bindings()}
        translated={row['source'] for row in bindings_and_keys()[0]+inline_bindings()}
        for source,english in (('午前','AM'),('午後','PM')):
            self.assertEqual(direct[source]['eng'],english)
            self.assertEqual(direct[source]['paths'],['include/ui_calendar_locale_data.inc'])
            self.assertNotIn(source,translated)

    def test_legacy_events_keep_numeric_tokens_in_single_player(self):
        source='A {IsMultiplayer:shared|individual} {Gold} {AromaPrinciple}'
        self.assertEqual(legacy_single_player(source,'principle'),'A individual {Gold} principle')
        self.assertEqual(legacy_single_player('{IsMultiplayer:shared|}{MaxHp}',''),'{MaxHp}')
        with self.assertRaises(ValueError):legacy_single_player('{Unbound}','')

    def test_language_subsets_keep_runtime_symbols(self):
        symbols=runtime_symbols()
        # Source checkboxes now use their original image assets. The removed
        # text checkmark must not force an unused glyph into every locale.
        self.assertTrue(set('▼–←→↑↓‹›×…−·').issubset(set(glyph_alphabet(['Synthetic English']))))
        self.assertTrue(symbols.issubset(set(glyph_alphabet(['Synthetic alternative']))))
        self.assertNotIn('\n',glyph_alphabet(['Synthetic\ntext']))


if __name__=='__main__':unittest.main()
