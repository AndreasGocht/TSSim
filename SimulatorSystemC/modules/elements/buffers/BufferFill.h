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
 * @file Buffer to simulate a Buffer, that just empties after it is fully filled.
 */


#ifndef BUFFERFILL_H_
#define BUFFERFILL_H_

#include "systemc.h"
#include "framework/CsvTrace.h"
#include <stdint.h>
#include <memory>

class BufferFillOutIf :  virtual public sc_interface {
public:
    virtual void write(uint8_t*) = 0;          // blocking write
protected:
    BufferFillOutIf() {
    };
private:
    BufferFillOutIf (const BufferFillOutIf&);      // disable copy
    BufferFillOutIf& operator= (const BufferFillOutIf&); // disable
};

class BufferFillInIf :  virtual public sc_interface {
public:
    virtual void read(uint8_t*&) = 0;          // blocking read
    virtual uint8_t* read() = 0;

protected:
    BufferFillInIf () {
    };
private:
    BufferFillInIf (const BufferFillInIf&);            // disable copy
    BufferFillInIf & operator= (const BufferFillInIf&); // disable =
};

class BufferFill
    : public sc_core::sc_prim_channel
    , public BufferFillOutIf
        , public BufferFillInIf {
public:
    BufferFill(const char* name);
    virtual ~BufferFill();

    void reset();

    void write(uint8_t* c);
    void read(uint8_t*& c);
    uint8_t* read();

    int fill = 0;
    int rd = 0;

private:
    void loadConfig();

    int size;                 // size
    uint8_t** buf;              // buffer
    bool readState;

    sc_event dataFullEvent;
    sc_event dataEmptyEvent;

    std::shared_ptr<CsvTrace> m_csvTrace;

};

#endif /* BUFFERFILL_H_ */
