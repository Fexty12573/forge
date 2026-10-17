"""Reject ARM32 modules that ask nnrtld to relocate RX or R pages."""
import struct


def check(image, segments, dynamic):
    tags = {}
    for pos in range(dynamic, len(image), 8):
        tag, value = struct.unpack_from('<II', image, pos)
        if tag == 0:
            break
        tags[tag] = value
    else:
        raise ValueError('Unterminated dynamic table')
    if 22 in tags or tags.get(30, 0) & 4:
        raise ValueError('DT_TEXTREL/DF_TEXTREL: loader would write read-only pages')
    if any(tags.get(tag, 0) for tag in (7, 8, 35, 36)):
        raise ValueError('Unsupported RELA/RELR relocation format')
    if tags.get(19, 8) != 8 or tags.get(20, 17) != 17:
        raise ValueError('Expected ARM32 REL entries')
    # MHGU overrides new/delete with an engine heap unavailable during DT_INIT.
    symbol_count = struct.unpack_from('<I', image, tags[4] + 4)[0]
    for index in range(symbol_count):
        name, _, _, _, _, section = struct.unpack_from('<IIIBBH', image, tags[6] + 16 * index)
        start = tags[5] + name
        name = bytes(image[start:image.index(0, start)]).decode()
        if not section and name.startswith(('_Znw', '_Zna', '_Zdl', '_Zda')):
            raise ValueError(f'Unsafe game allocator import during Forge initialization: {name}')
    counts = {'relative': 0, 'absolute': 0, 'global': 0, 'jump': 0}
    names = {23: 'relative', 2: 'absolute', 21: 'global', 22: 'jump'}
    for table_tag, size_tag in ((17, 18), (23, 2)):
        start, size = tags.get(table_tag, 0), tags.get(size_tag, 0)
        if size % 8 or start + size > len(image):
            raise ValueError('Invalid relocation table bounds')
        for pos in range(start, start + size, 8):
            target, info = struct.unpack_from('<II', image, pos)
            kind = info & 255
            if kind == 0:
                continue
            if kind not in names:
                raise ValueError(f'Unsupported ARM relocation {kind}')
            if not any(flags & 2 and addr <= target and target + 4 <= addr + memsz
                       for addr, memsz, flags in segments):
                raise ValueError(f'Relocation target 0x{target:x} is outside writable data/BSS')
            counts[names[kind]] += 1
    return {'dt_textrel': False, 'readonly_targets': 0, 'external_cpp_allocators': 0, 'counts': counts,
            'total_writes': sum(counts.values())}


def check_elf(elf, headers):
    loads = [h for h in headers if h[0] == 1]
    image = bytearray(max(h[2] + h[5] for h in loads))
    for _, off, addr, _, filesz, _, _, _ in loads:
        image[addr:addr + filesz] = elf[off:off + filesz]
    dynamic = next(h[2] for h in headers if h[0] == 2)
    return check(image, [(h[2], h[5], h[6]) for h in loads], dynamic)


def check_nso(nso):
    if nso[:4] != b'NSO0' or struct.unpack_from('<I', nso, 12)[0] & 7:
        raise ValueError('Expected uncompressed NSO')
    loads = [struct.unpack_from('<III', nso, 0x10 + 16 * i) for i in range(3)]
    bss = struct.unpack_from('<I', nso, 0x3c)[0]
    image = bytearray(loads[2][1] + loads[2][2] + bss)
    for off, addr, size in loads:
        image[addr:addr + size] = nso[off:off + size]
    mod0 = struct.unpack_from('<I', image, 4)[0]
    if image[mod0:mod0 + 4] != b'MOD0':
        raise ValueError('Missing MOD0')
    dynamic = mod0 + struct.unpack_from('<i', image, mod0 + 4)[0]
    segments = [(addr, size + (bss if i == 2 else 0), flags)
                for i, ((_, addr, size), flags) in enumerate(zip(loads, (5, 4, 6)))]
    return check(image, segments, dynamic)
