"""The calculator allocation includes the Zehn prefix, not only ELF memory."""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from zehn import build


def image(memory_end: int) -> bytes:
    """A four-byte ARM text image with a tail BSS of the requested extent."""
    names = b'\0.text\0.bss\0.shstrtab\0'
    names_offset = 56
    sections_offset = (names_offset + len(names) + 3) & ~3
    header = b'\x7fELF\x01\x01\x01' + bytes(9)
    header += struct.pack('<HH5I6H', 2, 40, 1, 0, 0, sections_offset, 0,
                          52, 0, 0, 40, 4, 3)
    payload = header + struct.pack('<I', 0xe12fff1e) + names
    payload += bytes(sections_offset - len(payload))
    sections = [
        (0,) * 10,
        (1, 1, 6, 0, 52, 4, 0, 0, 4, 0),
        (7, 8, 3, 8, 0, memory_end - 8, 0, 0, 8, 0),
        (12, 3, 0, 0, names_offset, len(names), 0, 0, 1, 0),
    ]
    return payload + b''.join(struct.pack('<10I', *row) for row in sections)


class ZehnAllocationTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.source = Path(self.directory.name) / 'input.elf'
        self.target = Path(self.directory.name) / 'output.tns'
        self.make(128)
        # Read the actual serialized allocation convention. This remains
        # valid if the flag list or metadata length changes later.
        self.prefix = struct.unpack_from('<I', self.target.read_bytes(), 24)[0] - 128
        self.assertGreater(self.prefix, 0)

    def make(self, memory_end):
        self.source.write_bytes(image(memory_end))
        with redirect_stdout(StringIO()):
            build(self.source, self.target)

    def test_total_allocation_at_limit_is_accepted(self):
        self.make(0x1000000 - self.prefix)
        self.assertEqual(struct.unpack_from('<I', self.target.read_bytes(), 24)[0], 0x1000000)

    def test_prefix_pushes_valid_image_over_limit_without_replacing_output(self):
        previous = self.target.read_bytes()
        memory_end = 0x1000000 - self.prefix + 1
        self.assertLess(memory_end, 0x1000000)
        with self.assertRaisesRegex(ValueError, 'Zehn allocation exceeds 16 MiB'):
            self.make(memory_end)
        self.assertEqual(self.target.read_bytes(), previous)


if __name__ == '__main__':
    unittest.main()
