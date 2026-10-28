"""The tiny history append uses real scene-face encoding and skips old glyphs."""
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import append_ui9_history_glyphs as patch

class HistoryHeaderGlyphs(unittest.TestCase):
    def test_existing_face_is_preserved_and_only_missing_header_added(self):
        class Source:
            strings=[(0,0)]*8704
            strings[8253]=(1,1);strings[8254]=(2,1);strings[8272]=(3,1)
            strings[3446]=(4,1)
            glyphs=[(65,100,101,1,1,1,0),(66,101,165,1,1,1,0)]
            def block(self,at,n):return {1:b'A',2:b'B',3:b'C',4:b'A'}[at]
        class Font:
            coverage={65,66,67}
            def glyph(self,cp,px,style):return (cp,px|((style&3)<<6)|((style&4)<<3),1,1,1,0,b'\xff')
        source=Source();font=Font();rows=patch.missing_glyphs(source,font)
        self.assertEqual([(row[0],row[1]) for row in rows],[(66,101),(67,101),(65,102)])
        source.glyphs+= [(cp,0,px,w,h,advance,encoded) for cp,px,w,h,advance,encoded,_raw in rows]
        self.assertEqual(patch.missing_glyphs(source,font),[])
        self.assertEqual(source.glyphs[0],(65,100,101,1,1,1,0))

if __name__=='__main__':unittest.main()
