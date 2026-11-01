import unittest,struct
import numpy as np
import neow_fire_ui23 as n
class NeowFireTest(unittest.TestCase):
 def test_patch_keys_and_sparse_literals(self):
  rng=np.random.default_rng(31);previous=bytes(n.PIXELS)
  for frame in range(80):
   row=bytearray(previous)
   for at in rng.integers(0,n.PIXELS,50):row[int(at)]=int(rng.integers(256))
   row[100:200]=bytes([frame])*100
   ref=bytes(n.PIXELS)if frame%32==0 else previous;raw=n.encode(bytes(row),ref,frame)
   self.assertEqual(n.decode(raw,ref,frame),row);previous=bytes(row)
   with self.assertRaises(ValueError):n.decode(raw[:-1],ref,frame)
   bad=bytearray(raw);struct.pack_into('<H',bad,4,frame+1)
   with self.assertRaises(ValueError):n.decode(bad,ref,frame)
 def test_cyclic_hold_bound_and_endpoints(self):
  source=np.empty((n.FRAMES,n.PIXELS),'u1')
  for i in range(n.FRAMES):source[i]=np.uint8((np.arange(n.PIXELS,dtype='u4')+i//5)%256)
  selected,_=n.hold(source);self.assertLessEqual(int(np.max(abs(selected.astype('i2')-source.astype('i2')))),8)
  self.assertTrue(np.all(selected[source==0]==0));self.assertTrue(np.all(selected[source==255]==255))
 def test_reduced_payload_alias(self):
  from pathlib import Path
  from tempfile import TemporaryDirectory
  import content_tiles as t
  from tanx_scene_ui23 import group_images
  raw=n.encode(bytes([127])*n.PIXELS,bytes(n.PIXELS),0)
  with TemporaryDirectory()as folder:
   root=Path(folder);(root/'mask.bin').write_bytes(raw);row=dict(width=256,height=len(raw)//512,format=0,file='mask.bin',bytes=len(raw),sha256=t.sha256(raw),raw_sha256=t.sha256(raw))
   art=group_images([row]*n.FRAMES,root,group_bytes=256*1024)
   self.assertEqual(len(set(art['entries'])),1);self.assertLess(len(art['payload']),len(raw));self.assertLessEqual(art['report']['maximum_group_bytes'],len(raw))
if __name__=='__main__':unittest.main()
