from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import build_converter_bootloader as b


class BootloaderCompatibility(unittest.TestCase):
    def test_patch_is_exact_source_bound_and_changes_only_two_flags(self):
        source=('before\n'+''.join(b.REMOVED)+'after\n').encode()
        with self.assertRaisesRegex(ValueError,'pinned6.16'):b.compatible_wscript(source)
        with patch.object(b,'WSCRIPT_SHA',b.sha(source)):
            actual=b.compatible_wscript(source)
            self.assertEqual(actual,b'before\n'+b'            # Unicorn2.1.4: documented non-CFG bootloader compatibility build.\n'*2+b'after\n')
            with self.assertRaisesRegex(ValueError,'pinned6.16'):b.compatible_wscript(source+b' ')

    def test_pe_flags_distinguish_cfg_without_hiding_aslr_or_nx(self):
        raw=bytearray(256);raw[:2]=b'MZ';struct.pack_into('<I',raw,60,64)
        raw[64:68]=b'PE\0\0';struct.pack_into('<H',raw,68,0x8664)
        struct.pack_into('<H',raw,88,0x20b);struct.pack_into('<HH',raw,156,3,0x4160)
        before=b.pe_properties(raw)
        self.assertTrue(before['control_flow_guard']);self.assertTrue(before['dynamic_base']);self.assertTrue(before['nx_compatible'])
        struct.pack_into('<H',raw,158,0x0160);after=b.pe_properties(raw)
        self.assertFalse(after['control_flow_guard']);self.assertTrue(after['dynamic_base']);self.assertTrue(after['nx_compatible'])
        self.assertTrue(after['high_entropy_va']);self.assertEqual(after['subsystem'],3)
        b.require_compatible_pe(after)
        with self.assertRaisesRegex(ValueError,'high-entropy'):b.require_compatible_pe(dict(after,high_entropy_va=False))
        with self.assertRaisesRegex(ValueError,'only CFG'):b.require_compatible_pe(before)
        struct.pack_into('<H',raw,68,0x14c)
        with self.assertRaisesRegex(ValueError,'AMD64'):b.pe_properties(raw)

if __name__=='__main__':unittest.main()
