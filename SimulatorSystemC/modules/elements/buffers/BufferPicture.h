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
 * @file Buffer to simulate a picture Buffer behavior.
 */

#ifndef BUFFERDECODE_H_
#define BUFFERDECODE_H_

#include "systemc.h"
#include "framework/CsvTrace.h"
#include <stdint.h>
#include <memory>
#include <map>
#include <list>
#include <tuple>



class BufferPictureOutIf :  virtual public sc_interface {
public:
    virtual int64_t write(uint8_t*, int64_t pts, int size) = 0;          // blocking write
    virtual void finished(std::list<int64_t> keys) = 0;          // no need on the writing side for thease frames
protected:
    BufferPictureOutIf() {
    };
private:
    BufferPictureOutIf (const BufferPictureOutIf&);      // disable copy
    BufferPictureOutIf& operator= (const BufferPictureOutIf&); // disable
};

class BufferPictureInIf :  virtual public sc_interface {
public:
    virtual void nbread(uint8_t*&, int64_t pt, int& size) = 0;          // noblocking read

protected:
    BufferPictureInIf () {
    };
private:
    BufferPictureInIf (const BufferPictureInIf&);            // disable copy
    BufferPictureInIf & operator= (const BufferPictureInIf&); // disable =
};

//systemC works with in and out ports. The ports uses kind of Interfaces.
//So the multiple inheritance is used to implement these interfaces.
class BufferPicture
    : public sc_core::sc_prim_channel
    , public BufferPictureOutIf
    , public BufferPictureInIf {
public:
    BufferPicture(const char* name);
    ~BufferPicture();

    void reset();

    int64_t write(uint8_t* c, int64_t pts, int size);
    void finished(std::list<int64_t> keys);

    void nbread(uint8_t*& c, int64_t pt, int& size);

    int fill;
    int64_t lastRequest = 0;

private:
    void loadConfig();

    void finish(int64_t key);
    /*
     * interfaces to finish:
     */
    void finish(std::list<int64_t> it);


    int m_size;                 // size

    std::map<int64_t, std::tuple<uint8_t*, int, int> > buf;/*!< this map holds each frame an the information in the order <pts,<frame,size,reference counter>> */
    bool readState;

    sc_event bufferElementDeleteEvent;
    sc_event bufferElementWriteEvent;

    std::shared_ptr<CsvTrace> m_csvTrace;

};


#endif /* BUFFERDECODE_H_ */
