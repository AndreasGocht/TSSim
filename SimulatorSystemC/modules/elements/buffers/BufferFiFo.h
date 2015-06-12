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
 * @BufferFiFo.h This Buffer is a Simple fifo implementation. It takes up to size elements. The size of each element is not observed.
 *
 */

#ifndef BUFFERFIFO_H_
#define BUFFERFIFO_H_

#include "systemc.h"
#include "framework/CsvTrace.h"
#include <stdint.h>
#include <memory>
#include <list>

class BufferFiFoOutIf :  virtual public sc_interface {
public:
    virtual void write(uint8_t*,int) = 0;          // blocking write
    virtual bool nbwrite(uint8_t*,int) = 0;          // noblocking write

protected:
    BufferFiFoOutIf() {
    };
private:
    BufferFiFoOutIf (const BufferFiFoOutIf&);      // disable copy
    BufferFiFoOutIf& operator= (const BufferFiFoOutIf&); // disable
};

class BufferFiFoInIf :  virtual public sc_interface {
public:
    virtual void read(uint8_t*&,int&) = 0;          // blocking read
    virtual uint8_t* read(int&) = 0;

protected:
    BufferFiFoInIf() {
    };
private:
    BufferFiFoInIf(const BufferFiFoInIf&);            // disable copy
    BufferFiFoInIf& operator= (const BufferFiFoInIf&); // disable =
};

class BufferFiFo
    : public sc_core::sc_prim_channel
    , public BufferFiFoOutIf
    , public BufferFiFoInIf {
public:
    BufferFiFo(const char* name);
    virtual ~BufferFiFo();

    void reset();

    void write(uint8_t* c, int size);
    bool nbwrite(uint8_t* c, int size);

    void read(uint8_t*& c, int& size);
    uint8_t* read(int& size);

    int fill;

private:
    void loadConfig();

    int size;                 // size
    std::list<std::pair<uint8_t*,int>> buf;              // buffer
    bool readState;

    sc_event dataReadEvent;
    sc_event dataWriteEvent;

    std::shared_ptr<CsvTrace> m_csvTrace;

};
#endif /* BUFFERFIFO_H_ */
