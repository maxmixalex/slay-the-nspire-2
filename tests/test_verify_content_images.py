"""Parallel oracle orchestration contract; synthetic protocol, no game pixels.

These tests exercise subprocess-result handling and complete-range accounting.
Only the separately executed native resource oracle proves actual image bytes.
"""
from contextlib import redirect_stdout
import io
import json
from pathlib import Path
import runpy
import struct
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock

ROOT=Path(__file__).resolve().parents[1]
SCRIPT=ROOT/'tools/verify_content_images.py'


class VerifyContentImagesTest(unittest.TestCase):
    def invoke(self,folder,*,failure=None):
        images=73;base=40+images*16;costs=base+64
        data=bytearray(costs+images)
        struct.pack_into('<4s9I',data,0,b'NSP2',45,0x41cef1ea,len(data),images,0,0,0,0,0)
        struct.pack_into('<4s',data,base,b'TIL1')
        struct.pack_into('<II',data,base+56,costs,images)
        data[costs:]=bytes([16])*images
        pack=folder/'synthetic.nsp';pack.write_bytes(data)
        output=folder/'output';lock=threading.Lock();active=peak=0;calls=[]

        def protocol(command,*,stdout,stderr):
            nonlocal active,peak
            position=command.index('--images')
            first,count=map(int,command[position+1:position+3])
            self.assertEqual(command[-1],'--preload')
            self.assertEqual(command[2:4],[str(pack),str(pack)])
            with lock:
                active+=1;peak=max(peak,active);calls.append((first,count))
            try:
                time.sleep(.01)
                measured=count-1 if failure=='wrong_count' and first==0 else count
                stdout.write(f'100 checks; {measured} of {images} images ({count*100} decoded bytes)\n')
                if failure!='missing_preload' or first!=0:
                    stdout.write(f'incremental catalog: {count} images; {count*2} steps; maximum 2 steps\n')
                return subprocess.CompletedProcess(command,1 if failure=='failed_process' and first==0 else 0)
            finally:
                with lock:active-=1

        argv=[str(SCRIPT),'--binary',sys.executable,'--pack',str(pack),'--reference',str(pack),
              '--output',str(output),'--spool-mib','1','--workers','4']
        with mock.patch.object(sys,'argv',argv),mock.patch.object(subprocess,'run',protocol),redirect_stdout(io.StringIO()):
            runpy.run_path(str(SCRIPT),run_name='__main__')
        return json.loads((output/'exact-image-receipt.json').read_text()),calls,peak

    def test_four_workers_cover_every_requested_image_exactly_once(self):
        with tempfile.TemporaryDirectory() as folder:
            receipt,calls,peak=self.invoke(Path(folder))
            self.assertEqual(peak,4)
            self.assertTrue(receipt['complete'])
            self.assertEqual(receipt['workers'],4)
            self.assertEqual(receipt['aggregate_spool_bound'],4*1024*1024)
            self.assertEqual(receipt['images'],73)
            self.assertEqual(receipt['decoded_bytes'],7300)
            cursor=0
            for first,count in sorted(calls):
                self.assertEqual(first,cursor);cursor+=count
            self.assertEqual(cursor,73)
            self.assertEqual([(r['first'],r['count']) for r in receipt['ranges']],sorted(calls))
            self.assertTrue(all(r['maximum_spool_bytes']<=1024*1024 for r in receipt['ranges']))

    def test_failed_or_incomplete_native_protocol_never_marks_complete(self):
        for failure in ('failed_process','missing_preload','wrong_count'):
            with self.subTest(failure=failure),tempfile.TemporaryDirectory() as folder:
                with self.assertRaises(ValueError):self.invoke(Path(folder),failure=failure)
                receipt=json.loads((Path(folder)/'output/exact-image-receipt.json').read_text())
                self.assertFalse(receipt['complete'])


if __name__=='__main__':unittest.main()
