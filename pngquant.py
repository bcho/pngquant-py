#coding: utf-8

from os import path

import cffi


CURRENT_DIR = path.dirname(path.abspath(__file__))
PNGQUANT_PATH = path.join(CURRENT_DIR, 'pngquant')
PNGQUANT_SOURCE = map(lambda part: path.join(PNGQUANT_PATH, 'lib', part),
                      ['pam.c', 'mediancut.c', 'blur.c', 'viter.c',
                       'mempool.c', 'nearest.c', 'libimagequant.c'])
PNGQUANT_SOURCE += map(lambda part: path.join(PNGQUANT_PATH, part),
                       ['rwpng.c'])
PNGQUANT_SOURCE += map(lambda part: path.join(CURRENT_DIR, part),
                       ['pngquant_tiny.c'])
PNGQUANT_INCLUDES = ['/usr/local/lib/', '/usr/lib/']
PNGQUANT_COMPILE_ARGS = ['-Wall', '-Wno-unknown-pragmas', '-O3', '-DNDEBUG',
                         '-std=c99', '-ffast-math', '-funroll-loops',
                         '-fomit-frame-pointer']
PNGQUANT_LINK_ARGS = ['-lpng', '-lz', '-lm']


ffi = cffi.FFI()
ffi.cdef('''
void fclose(FILE *);
FILE *fmemopen(void *, size_t, const char *);
void pngquant_tiny(FILE *, FILE *);
''')
impl = ffi.verify(sources=PNGQUANT_SOURCE, language='c',
                  include_dirs=PNGQUANT_INCLUDES,
                  extra_compile_args=PNGQUANT_COMPILE_ARGS,
                  extra_link_args=PNGQUANT_LINK_ARGS)


def tiny(source, width, height):
    '''Compress png format image.

    Usage:

        with open('origin.png', 'rb') as origin:
            rv = tiny(origin.read(), origin_img_width, origin_img_height)
            with open('compressed.png', 'wb') as compressed:
                compressed.write(rv)

    :param source: source image content
    :param width, height: source image width & height
    '''
    # TODO automatically get size
    size = width * height * 4

    # Create `FILE` pointer for input & output buffer.
    source_buf = ffi.new('char []', source)
    source_fp = impl.fmemopen(source_buf, len(source), 'r')
    dest_buf = ffi.new('char []', size)
    dest_fp = impl.fmemopen(dest_buf, size, 'w')

    impl.pngquant_tiny(source_fp, dest_fp)
    impl.fclose(source_fp)
    impl.fclose(dest_fp)

    # Remove useless "zero boundry" in output buffer's end
    # TODO use FILE io, remove double check
    while size > 0:
        if dest_buf[size - 1] != '\x00':
            break
        size = size - 1

    return ''.join(dest_buf[0:size])
