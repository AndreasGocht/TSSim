/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2015 Digisoft.tv Ltd.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @file Buffer to simulate the behavior of a Video Decoder buffer.
 *
 * It works as a FiFo with frames that are given. But compared to a normal FiFo the filling calculation is
 * not done by the frames that are in the Buffer, but by the size of the given frames.
 */
#ifndef MODULES_ELEMENTS_BUFFERS_BUFFERDECODER_H_
#define MODULES_ELEMENTS_BUFFERS_BUFFERDECODER_H_

#include "systemc.h"
#include "framework/CsvTrace.h"
#include <stdint.h>
#include <list>
#include <memory>

class BufferDecoderOutIf :  virtual public sc_interface {
public:
    virtual void write(uint8_t*, int64_t, int) = 0;          // blocking write
protected:
    BufferDecoderOutIf() {
    };
private:
    BufferDecoderOutIf (const BufferDecoderOutIf&);      // disable copy
    BufferDecoderOutIf& operator= (const BufferDecoderOutIf&); // disable
};

class BufferDecoderInIf :  virtual public sc_interface {
public:
    virtual void read(uint8_t*&, int64_t&, int&) = 0;          // blocking read

protected:
    BufferDecoderInIf () {
    };
private:
    BufferDecoderInIf (const BufferDecoderInIf&);            // disable copy
    BufferDecoderInIf & operator= (const BufferDecoderInIf&); // disable =
};



class BufferDecoder
    : public sc_core::sc_prim_channel
    , public BufferDecoderInIf
    , public BufferDecoderOutIf
{
public:
    BufferDecoder(const char* name);
    virtual ~BufferDecoder();

    void reset();

    void write(uint8_t* buffer, int64_t pts, int size);
    void read(uint8_t*& buffer, int64_t& pts, int& size);
    double fillPercent();

    int getSize();

    int fill;


private:
    void loadConfig();

    int m_size;                 // size
    std::list<std::pair<uint8_t*, int>> m_bitstreamBuffer;
    std::list<uint64_t> m_ptsBuffer;

    sc_event m_dataReadEvent;
    sc_event m_dataWriteEvent;

    std::shared_ptr<CsvTrace> m_csvTrace;

};

#endif /* MODULES_ELEMENTS_BUFFERS_BUFFERVIDEODECODER_H_ */
