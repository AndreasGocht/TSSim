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

#include <modules/elements/buffers/BufferPicture.h>
#include "framework/Configuration.h"


#define MODULE_ID_STR "/digisoft/simulator/modules/elements/buffers/BufferPicture"
#define STC_WARPAROUND_OFFSET 500000000


BufferPicture::BufferPicture(const char* name)
    : sc_prim_channel(name)
{
    this->loadConfig();
    this->reset();
}

/** @brief load the configuration
 *
 */
void BufferPicture::loadConfig()
{
    Configuration& config = Configuration::getInstance();

    if (!config.HasMember(this->name())) {
        std::string message;
        message += "No Configuration found for: \"";
        message += this->name();
        message += "\"";
        SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
    }

    rapidjson::Value& s = config[this->name()];

    if (!s.HasMember("size") || !s["size"].IsInt()) {
        std::string message;
        message += "Malformed configuration of \"";
        message += this->name();
        message += "\". \"size\" is missing or no Int";
        SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
    }

    this->m_size = s["size"].GetInt();

    if (!s.HasMember("trace") || !s["trace"].IsBool()) {
        std::string message;
        message += "Malformed configuration of \"";
        message += this->name();
        message += "\". \"trace\" is missing or no Bool. This Module will not been logged";
        SC_REPORT_WARNING(MODULE_ID_STR, message.c_str());
    } else {
        if (s["trace"].GetBool()) {
            m_csvTrace = std::make_shared<CsvTrace>(config.dir());
            m_csvTrace->delta_cycles(true);
            m_csvTrace->trace(this->fill, std::string(this->name()).append(".fill"), "fill in elements");
        }
    }
}

/** @brief delete all remaining elements in the map
 *
 */
BufferPicture::~BufferPicture() {

    for (std::map<int64_t, std::tuple<uint8_t*, int, int> >::iterator it=buf.begin(); it!=buf.end(); ++it)
    {
        std::tuple<uint8_t*,int,int> elem = it->second;
        delete [] std::get<0>(elem);
        buf.erase(it);
        --fill;
    }

}

/** @brief reset the buffer
 *
 */
void BufferPicture::reset()
{
    fill = 0;
}

/** @brief save an element in the buffer
 *
 * this will save an element in the Buffer, and set the refence counter of each element to 2.
 * The former element should call @finished() when it no longer need the given elements itself,
 * so they could be deleted.
 *
 * @param c buffer to save
 * @param pts presentation time stamp
 * @param size of the elemet
 *
 * @return key to identify the element, to use with @finished()
 */
int64_t BufferPicture::write(uint8_t* c, int64_t pts, int size)
{
    if (fill == this->m_size) {
        wait(bufferElementDeleteEvent);
    }

    while (buf.count(pts)>0)
    {
        string message;
        message = "PTS:";
        message += to_string(pts);
        message += " already exist. Add 1 and save anyway";
        SC_REPORT_WARNING("/digisoft/simulator/BufferFill", message.c_str());
        pts++;
    }
    buf[pts] = std::make_tuple(c, size, 2);


    ++fill;
    bufferElementWriteEvent.notify();

    return pts;
}

/** @brief call this function when an element is no longer needed by the former element.
 *
 * this function will care about reducting the reference counter by one. It should calles with a list of elements that are no
 * longer needed.
 *
 * @param frames list of by write given keys
 *
 */
void BufferPicture::finished(std::list<int64_t> frames)
{
    for (std::list<int64_t>::iterator frame=frames.begin(); frame != frames.end(); ++frame)
    {
        this->finish(*frame);
    }
}

/** @brief nonblocking read. Read an element form the Buffer
 *
 * This funciton will read and remove the element with a pts nearest to pt
 * from the buffer, and delete all elements with a timestamp lower than this.
 *
 * @param[out] c buffer to fill with the element, null if none present
 * @param[in] requested timestamp
 * @param[out] size of c, 0 if there is nothing to retrun
 */
void BufferPicture::nbread(uint8_t*& c, int64_t pt, int& size)
{
    std::list<int64_t> toFinish;

    if(lastRequest>pt){
        SC_REPORT_WARNING(MODULE_ID_STR,"this request time is before the last request.\n"
                "Likely its an stc jump or warparound. If not there is something wrong.\n"
                "Will throw away all pictures after the last pts request");

        for (std::map<int64_t, std::tuple<uint8_t*, int, int> >::iterator it=buf.begin(); it!=buf.end(); ++it)
        {
            int64_t pts = it->first;
            if (pts > lastRequest)
            {
                toFinish.push_back(it->first);
            }
        }

    }

    if(fill == 0)
    {
        c = NULL;
        size = 0;
    }
    else
    {
        int64_t resultTime = 0;
        std::map<int64_t, std::tuple<uint8_t*, int, int> >::iterator result = buf.end();
        for (std::map<int64_t, std::tuple<uint8_t*, int, int> >::iterator it=buf.begin(); it!=buf.end(); ++it)
        {
            int64_t pts = it->first;
            /*
             * pts>(pt-STC_WARPAROUND_OFFSET) don't care about elements that are too early
             */
            if (pts < pt && pts > (pt - STC_WARPAROUND_OFFSET))
            {
                // get the packet nearest to the pt
                if(resultTime < pts){
                    resultTime = pts;
                    toFinish.push_back(result->first);
                    result = it;
                }
                else
                {
                    toFinish.push_back(it->first);
                }
            }
        }

        if (result != buf.end())
        {
            std::tuple<uint8_t*, int, int> elem = result->second;
            size = std::get<1>(elem);
            c = new uint8_t[size];
            memcpy(c, std::get<0>(elem), size);
            toFinish.push_back(result->first);
        }
        else
        {
            size = 0;
            c = NULL;
        }

    }

    this->finished(toFinish);
    lastRequest = pt;
}

/** @brief interface for iterators to @finish()
 *
 */
void BufferPicture::finish(std::list<int64_t> toFinish)
{
    for(std::list<int64_t>::iterator it = toFinish.begin(); it != toFinish.end(); ++it)
    {
        finish(*it);
    }
}

/** @brief function to call to delete an element
 *
 * This function will take care of the reference counter. If the counter drops to 0
 * there is no need anymore for this element. The decoder has finished its work, and
 * the picture as already been removed from the buffer
 *
 * @param key for the element to remove. it is basically the pts of the vaule in the map.
 *
 */
void BufferPicture::finish(int64_t key)
{

    std::tuple<uint8_t*, int, int> savedFrame = buf[key];

    /*
     * reduce counter
     */
    --std::get<2>(savedFrame);

    /*
     * if counter at 0 delete
     */
    if (std::get<2>(savedFrame) == 0)
    {
        delete [] std::get<0>(savedFrame);
        this->buf.erase(key);
        --this->fill;
        bufferElementDeleteEvent.notify();
    }
    else
    {
        this->buf[key] = savedFrame;
    }
}

#undef MODULE_ID_STR
