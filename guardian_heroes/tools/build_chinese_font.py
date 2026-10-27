#!/usr/bin/env python3
"""Extend Guardian Heroes' native font XPRC, preserving existing glyphs and pixels."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile


def decompress(data):
    expected, packed = struct.unpack_from('>II', data)
    assert packed == len(data) - 8
    output = bytearray()
    block = bytearray()
    pos = 8
    while pos < len(data):
        control = data[pos]
        pos += 1
        if not control:
            output += block + bytes(-len(block) % 4)
            block.clear()
        elif control < 32:
            block += data[pos:pos + control]
            pos += control
        else:
            if control & 128:
                distance, length = ((control >> 2) & 31) + 1, (control & 3) + 1
            else:
                word = control * 256 + data[pos]
                pos += 1
                if control & 64:
                    distance, length = ((word >> 4) & 1023) + 1, (word & 15) + 2
                else:
                    distance, length = ((word >> 9) & 15) + 1, (word & 511) + 2
            assert distance <= len(block)
            for _ in range(length):
                block.append(block[-distance])
        assert len(block) <= 2048
    assert not block and len(output) == expected
    return bytes(output)


def compress(data):
    # Blocks are independent; the guest decoder aligns each block to four bytes.
    assert len(data) % 4 == 0
    output = bytearray()
    for start in range(0, len(data), 2048):
        block = data[start:start + 2048]
        pos = 0
        literals = bytearray()
        def flush():
            if literals:
                output.append(len(literals))
                output.extend(literals)
                literals.clear()
        while pos < len(block):
            # Long nearby repeats account for the empty atlas and glyph interiors.
            best_length, best_distance = 0, 0
            for distance in range(1, min(pos, 16) + 1):
                length = 0
                limit = min(513, len(block) - pos)
                while length < limit and block[pos + length] == block[pos + length - distance]:
                    length += 1
                if length > best_length:
                    best_length, best_distance = length, distance
            if best_length >= 3:
                flush()
                word = 0x2000 | ((best_distance - 1) << 9) | (best_length - 2)
                output.extend(struct.pack('>H', word))
                pos += best_length
            else:
                literals.append(block[pos])
                pos += 1
                if len(literals) == 31:
                    flush()
        flush()
        output.append(0)
    return struct.pack('>II', len(data), len(output)) + output


def resource_entries(data):
    count = struct.unpack_from('>I', data, 12)[0]
    entries = []
    for i in range(count):
        tag, offset, size, name = struct.unpack_from('>4sIII', data, 16 + 16 * i)
        name = data[12 + name:].split(b'\0', 1)[0].decode('ascii')
        entries.append({'tag': tag, 'offset': offset, 'size': size, 'name': name,
                        'data': data[12 + offset:12 + offset + size]})
    return entries


def corpus_characters(paths):
    chars = set()
    def visit(value):
        if isinstance(value, dict):
            for key, item in value.items():
                if key in ('zh', 'translation', 'chinese') and isinstance(item, str):
                    chars.update(ord(char) for char in item if 0x80 <= ord(char) <= 0xFFEF)
                else:
                    visit(item)
        elif isinstance(value, list):
            for item in value:
                visit(item)
    for path in paths:
        visit(json.loads(path.read_text()))
    return chars


def extend_font(metrics, texture, texture_bytes, wanted, renderer, temporary):
    version, line_height = struct.unpack_from('>If', metrics)
    assert version == 5
    maximum = struct.unpack_from('>H', metrics, 20)[0]
    table = list(struct.unpack_from(f'>{maximum + 1}H', metrics, 22))
    count_pos = 24 + maximum * 2
    count = struct.unpack_from('>I', metrics, count_pos)[0]
    glyphs = metrics[count_pos + 4:count_pos + 4 + count * 16]
    # Redraw Chinese and full-width punctuation even if the Japanese atlas has
    # that character, so a sentence cannot mix native and new font weights.
    missing = sorted(code for code in wanted if code >= len(table) or table[code] == 0
                     or 0x3000 <= code <= 0x9FFF or 0xFF00 <= code <= 0xFFEF)
    if not missing:
        return metrics, texture, texture_bytes, {'added': 0}
    assert count + len(missing) <= 65535
    height = round(line_height)
    glyph_width = height - 2
    fetch = list(struct.unpack_from('>6I', texture, 28))
    assert fetch[0] >> 31 == 0 and fetch[1] & 63 == 15
    old_width, old_height = (fetch[2] & 8191) + 1, ((fetch[2] >> 13) & 8191) + 1
    old_pitch = ((fetch[0] >> 22) & 511) * 32
    width = old_width
    columns = width // (glyph_width + 1)
    new_height = old_height + ((len(missing) + columns - 1) // columns) * (height + 1)
    assert new_height <= 8192, (height, len(missing), new_height)
    atlas = bytearray(width * ((new_height + 31) // 32 * 32) * 2)
    # Row-stride changes leave original coordinates, coverage, and channel packing intact.
    for row in range(old_height):
        atlas[row * width * 2:(row * width + old_width) * 2] = texture_bytes[row * old_pitch * 2:(row * old_pitch + old_width) * 2]
    codes_path = temporary / f'codes-{height}.txt'
    pixels_path = temporary / f'pixels-{height}.raw'
    codes_path.write_text('\n'.join(map(str, missing)))
    pixels_path.touch()
    subprocess.run([str(renderer), str(codes_path), str(pixels_path), str(height)], check=True)
    pixels = pixels_path.read_bytes()
    assert len(pixels) == len(missing) * glyph_width * height
    extra = bytearray()
    table.extend([0] * (max(missing[-1], maximum) + 1 - len(table)))
    # Native regular glyphs use A4R4G4B4, with a zero channel selector.
    lut = [struct.pack('>H', (((value + 8) // 17) << 12) | 0xFFF) if value >= 9 else b'\0\0' for value in range(256)]
    for index, code in enumerate(missing):
        x = index % columns * (glyph_width + 1)
        y = old_height + index // columns * (height + 1)
        cell = pixels[index * glyph_width * height:(index + 1) * glyph_width * height]
        for row in range(height):
            offset = ((y + row) * width + x) * 2
            atlas[offset:offset + glyph_width * 2] = b''.join(lut[p] for p in cell[row * glyph_width:(row + 1) * glyph_width])
        table[code] = count + index
        extra += struct.pack('>8H', x, y, x + glyph_width, y + height, 0, glyph_width, 0, 0)
    rebuilt = (metrics[:20] + struct.pack('>H', len(table) - 1) +
               struct.pack(f'>{len(table)}H', *table) + struct.pack('>I', count + len(missing)) + glyphs + extra)
    rebuilt += bytes(-len(rebuilt) % 4)
    assert rebuilt[:20] == metrics[:20]
    new_glyph_start = 28 + (len(table) - 1) * 2
    assert rebuilt[new_glyph_start:new_glyph_start + len(glyphs)] == glyphs
    for code in range(maximum + 1):
        old_index = struct.unpack_from('>H', metrics, 22 + code * 2)[0]
        assert not old_index or code in missing or table[code] == old_index
    for row in range(old_height):
        assert atlas[row * width * 2:(row * width + old_width) * 2] == texture_bytes[row * old_pitch * 2:(row * old_pitch + old_width) * 2]
    fetch[0] = (fetch[0] & ~(511 << 22)) | ((width // 32) << 22)
    fetch[2] = (width - 1) | ((new_height - 1) << 13)
    texture = texture[:28] + struct.pack('>6I', *fetch)
    return rebuilt, texture, bytes(atlas), {'added': len(missing), 'glyphs': count + len(missing), 'width': width, 'height': new_height}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path('guardian_heroes/localization/font_original.xprc'))
    parser.add_argument('--output', type=Path, default=Path('guardian_heroes/localization/font_zh.xprc'))
    parser.add_argument('--corpus', type=Path, nargs='*')
    args = parser.parse_args()
    source = decompress(args.source.read_bytes())
    entries = resource_entries(source)
    corpus = args.corpus if args.corpus is not None else sorted(Path('guardian_heroes/localization').glob('*zh*.json'))
    common = corpus_characters(corpus)
    data_start = 12 + struct.unpack_from('>I', source, 4)[0]
    textures = bytearray()
    report = {}
    with tempfile.TemporaryDirectory(prefix='guardian-font-') as tmp:
        temporary = Path(tmp)
        renderer = temporary / 'render-glyphs'
        subprocess.run(['swiftc', '-O', str(Path(__file__).with_name('render_chinese_glyphs.swift')), '-o', str(renderer)], check=True)
        for i in range(0, len(entries), 2):
            texture, font = entries[i:i + 2]
            old_base = struct.unpack_from('>I', texture['data'], 32)[0] & 0xFFFFF000
            next_base = (struct.unpack_from('>I', entries[i + 2]['data'], 32)[0] & 0xFFFFF000) if i + 2 < len(entries) else len(source) - data_start
            pixels = source[data_start + old_base:data_start + next_base]
            # Keep translated menu/dialogue text in the default atlas and
            # preserve the specialized title/name atlases at their original sizes.
            wanted = common if font['name'] == 'DataDefault' else set()
            metrics, desc, pixels, result = extend_font(font['data'], texture['data'], pixels, wanted, renderer, temporary)
            font['data'] = metrics
            assert len(textures) % 4096 == 0
            desc = bytearray(desc)
            old_fetch = struct.unpack_from('>I', desc, 32)[0]
            struct.pack_into('>I', desc, 32, len(textures) | (old_fetch & 4095))
            texture['data'] = bytes(desc)
            textures += pixels + bytes(-len(pixels) % 4096)
            report[font['name']] = result
    header = bytearray(source[12:12 + min(entry['offset'] for entry in entries)])
    for index, entry in enumerate(entries):
        header += bytes(-len(header) % 4)
        struct.pack_into('>II', header, 4 + index * 16 + 4, len(header), len(entry['data']))
        header += entry['data']
    header += bytes(-(12 + len(header)) % 4096)
    rebuilt = b'XPR2' + struct.pack('>II', len(header), len(textures)) + header + textures
    packed = compress(rebuilt)
    # This detects encoder mistakes before an invalid native resource can ship.
    assert decompress(packed) == rebuilt
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(packed)
    args.output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'output': str(args.output), 'packed_bytes': len(packed), 'fonts': report}, indent=2))

if __name__ == '__main__':
    main()
