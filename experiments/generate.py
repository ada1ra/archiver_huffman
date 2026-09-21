import os
import zipfile

INPUT = 'experiments/input'
os.makedirs(INPUT, exist_ok=True)

SIZES = {
    '1k':   1 * 1024,
    '10k':  10 * 1024,
    '100k': 100 * 1024,
    '1m':   1 * 1024 * 1024,
    '5m':   5 * 1024 * 1024,
}

PANGRAM = b'The quick brown fox jumps over the lazy dog. '
SRC_FILES = ['src/huffman.c', 'src/main.c', 'tests/tests.c']

def make_text(size):
    data = bytearray()
    while len(data) < size:
        data += PANGRAM
    return bytes(data[:size])

def make_code(size):
    pool = b''
    for path in SRC_FILES:
        if os.path.exists(path):
            with open(path, 'rb') as f:
                pool += f.read() + b'\n'
    if not pool:
        pool = PANGRAM
    data = bytearray()
    while len(data) < size:
        data += pool
    return bytes(data[:size])

def make_random(size):
    return os.urandom(size)

def make_repeated(size):
    return b'A' * size

def make_zip(size, name):
    tmp = f'/tmp/rnd_{name}.bin'
    with open(tmp, 'wb') as f:
        f.write(os.urandom(size))
    with zipfile.ZipFile(f'experiments/input/archive_{name}.zip', 'w', zipfile.ZIP_DEFLATED) as z:
        z.write(tmp, arcname='data.bin')
    os.remove(tmp)

make_zip(10 * 1024, '10k')
make_zip(100 * 1024, '100k')
make_zip(1 * 1024 * 1024, '1m')
for name, size in SIZES.items():
    files = {
        f'text_{name}.txt':     make_text(size),
        f'code_{name}.c':       make_code(size),
        f'random_{name}.bin':   make_random(size),
        f'repeated_{name}.bin': make_repeated(size),
    }
    for fname, data in files.items():
        path = os.path.join(INPUT, fname)
        with open(path, 'wb') as f:
            f.write(data)
        print(f'{path}: {len(data)} bytes')
