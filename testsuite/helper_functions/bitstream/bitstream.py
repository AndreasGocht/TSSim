'''
The MIT License (MIT)

Copyright (c) 2015 Digisoft.tv Ltd.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

@file Module to gernerate python bitstream bindings
 
This module parse the bitstream headerfiles for function definitiions.
This definitions are needed to use functions provided by libbitstream.so.1.0

The bistream libary could be used, using bs.<bitstream function>

bsNULL is provided to allow comparisions to NULL pointer
bsString is provided to allow conversations to python Strings

This module is pypy compatible.
'''
import os
import re
from cffi import FFI

def _cleanHeader(data):
    clean = data
    
    #remove #include
    clean = re.sub("#include[^\n]*\n", "", clean)
    #remove ifdef __cplusplus
    clean = re.sub('#ifdef __cplusplus[^#]*(#endif)', '', clean)
    #remove define (also about 2 lines)
    clean = re.sub("#define[^\n]*[(\\\n)][^\n]*\n", "", clean)
    #clean from psi p_psi_crc_table
    clean = re.sub("[^\n]*p_psi_crc_table[^;]*;", "", clean)
    #replace function contetn, beginning from the inners {}
    regStr = "[\n]?\{[^\}\{]*\}" 
    while re.search(regStr, clean):
        clean = re.sub(regStr, ';', clean)
    
    #replace rmaining #ifndef and #endif
    clean = re.sub("#ifndef[^\n]*\n", "", clean)
    clean = re.sub("#endif", '', clean)
    
    return clean

def _loadHeader(filename):
    with open(filename) as f:
        data = f.read()
    return data

_ffi = FFI()

_dataBitstream = _loadHeader(os.path.dirname(__file__) + "/bitstream.c")
_ffi.cdef(_cleanHeader(_dataBitstream))

_dataTs = _loadHeader(os.path.dirname(__file__) + "/../../../bitstream/mpeg/ts.h")
_ffi.cdef(_cleanHeader(_dataTs))

_dataPes = _loadHeader(os.path.dirname(__file__) + "/../../../bitstream/mpeg/pes.h")
_ffi.cdef(_cleanHeader(_dataPes))

_dataPsi = _loadHeader(os.path.dirname(__file__) + "/../../../bitstream/mpeg/psi/psi.h")
_ffi.cdef(_cleanHeader(_dataPsi))

_dataPat = _loadHeader(os.path.dirname(__file__) + "/../../../bitstream/mpeg/psi/pat.h")
_ffi.cdef(_cleanHeader(_dataPat))

_dataPmt = _loadHeader(os.path.dirname(__file__) + "/../../../bitstream/mpeg/psi/pmt.h")
_ffi.cdef(_cleanHeader(_dataPmt))


bs = _ffi.dlopen(os.path.dirname(__file__) + '/libbitstream.so.1.0')
bsNULL = _ffi.NULL
bsString = _ffi.string
