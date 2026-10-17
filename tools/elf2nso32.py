#!/usr/bin/env python3
"""Pack ARM32 ELF into hashed, uncompressed NSO segments using the standard library."""
import hashlib
from pathlib import Path
import struct
import sys
from check_relocations import check_elf, check_nso


def pack(elf):
    if elf[:7] != b'\x7fELF\x01\x01\x01':
        raise ValueError('Expected little-endian ELF32')
    if struct.unpack_from('<H', elf, 18)[0] != 40:
        raise ValueError('Expected ARM machine type')
    phoff, shoff = struct.unpack_from('<II', elf, 28)
    phsize, phcount, shsize, shcount = struct.unpack_from('<HHHH', elf, 42)
    headers = [struct.unpack_from('<8I', elf, phoff + i * phsize) for i in range(phcount)]
    loads = [h for h in headers if h[0] == 1]
    if len(loads) != 3 or [h[6] for h in loads] != [5, 4, 6]:
        raise ValueError('Expected RX/R/RW load segments')
    check_elf(elf, headers)
    header = bytearray(0x100)
    struct.pack_into('<4sIII', header, 0, b'NSO0', 0, 0, 0x38)
    build_id = None
    for i in range(shcount):
        sh = struct.unpack_from('<10I', elf, shoff + i * shsize)
        if sh[1] != 7:
            continue
        cursor, end = sh[4], sh[4] + sh[5]
        while cursor + 12 <= end:
            namesz, descsz, kind = struct.unpack_from('<III', elf, cursor)
            name = elf[cursor + 12:cursor + 12 + namesz]
            desc = cursor + 12 + ((namesz + 3) & ~3)
            if kind == 3 and name == b'GNU\x00':
                build_id = elf[desc:desc + descsz]
            cursor = desc + ((descsz + 3) & ~3)
    if not build_id or len(build_id) > 32:
        raise ValueError('Missing or oversized GNU build ID')
    header[0x40:0x40 + len(build_id)] = build_id
    result = bytearray(header)
    previous_end = 0
    for i, (_, off, addr, _, filesz, memsz, _, _) in enumerate(loads):
        if off + filesz > len(elf) or filesz > memsz:
            raise ValueError('Segment exceeds ELF bounds')
        if addr % 0x1000 or addr < previous_end:
            raise ValueError('Misaligned or overlapping memory segments')
        previous_end = (addr + memsz + 0xFFF) & ~0xFFF
        raw = elf[off:off + filesz]
        struct.pack_into('<4I', result, 0x10 + i * 0x10,
                         len(result), addr, filesz, memsz - filesz if i == 2 else 0)
        struct.pack_into('<I', result, 0x60 + i * 4, filesz)
        result[0xA0 + i * 32:0xC0 + i * 32] = hashlib.sha256(raw).digest()
        result.extend(raw)
    check_nso(result)
    return result


if __name__ == '__main__':
    source, target = map(Path, sys.argv[1:])
    target.write_bytes(pack(source.read_bytes()))
