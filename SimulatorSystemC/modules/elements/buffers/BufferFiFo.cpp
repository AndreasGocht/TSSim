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


#include <modules/elements/buffers/BufferFiFo.h>
#include "framework/Configuration.h"
#include "bitstream/mpeg/ts.h"

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/buffers/BufferFiFo"

/** @brief constructor. Load the Config and reset the buffer.
 *
 * @param name name of the element
 *
 */
BufferFiFo::BufferFiFo(const char* name)
    : sc_prim_channel(name)
{
    this->loadConfig();
    this->reset();
}
/** @brief load the config from a given  .json file.
 *
 */
void BufferFiFo::loadConfig()
{
    Configuration& config = Configuration::getInstance();

    if (!config.HasMember(this->name())) {
        std::string message;
        message += "No Configuration found for: \"";
        message += this->name();
        message += "\"";
        SC_REPORT_FATAL(MODULE_ID_STR , message.c_str());
    }

    rapidjson::Value& s = config[this->name()];

    if (!s.HasMember("size") || !s["size"].IsInt()) {
        std::string message;
        message += "Malformed configuration of \"";
        message += this->name();
        message += "\". \"size\" is missing or no Int";
        SC_REPORT_FATAL(MODULE_ID_STR , message.c_str());
    }
    this->size = s["size"].GetInt();

    if (!s.HasMember("trace") || !s["trace"].IsBool()) {
        std::string message;
        message += "Malformed configuration of \"";
        message += this->name();
        message += "\". \"trace\" is missing or no Bool. This Module will not been logged";
        SC_REPORT_WARNING(MODULE_ID_STR , message.c_str());
    } else {
        if (s["trace"].GetBool()) {
            m_csvTrace = std::make_shared<CsvTrace>(config.dir());
            m_csvTrace->delta_cycles(true);
            m_csvTrace->trace(this->fill, std::string(this->name()).append(".fill"), "fill in elements");
        }
    }
}

BufferFiFo::~BufferFiFo() {
}

/** @brief reset the Buffer
 *
 */
void BufferFiFo::reset()
{
    buf.empty();
    this->fill = 0;

}

/** @brief copy an Element to the Buffer
 *
 * The function will copy an element from the position c and the size size in Memeory to the Buffer. It will block if the FiFo is Full
 *
 * @param c pointer to the element
 * @param size size of the element
 *
 */
void BufferFiFo::write(uint8_t* c, int size)
{
    if (this->fill+1 > this->size) {
        wait(dataReadEvent);
    }

    uint8_t* buffToSave = new uint8_t[size];
    memcpy(buffToSave,c,size);

    buf.push_back(std::make_pair(buffToSave,size));
    fill ++;
    dataWriteEvent.notify();

}

/** @brief noblocking write
 *
 * The function will copy an element from the position c and the size size in Memeory to the Buffer. It will not block if the FiFo is Full.
 * Intead it will return true or false if the write was sucessful
 *
 * @param c pointer to the element
 * @param size size of the element
 *
 * return true if write was sucessfull, false otherwise.
 *
 */
bool BufferFiFo::nbwrite(uint8_t *c, int size)
{
    if (this->fill+1 > this->size) {
        return false;
    }

    uint8_t* buffToSave = new uint8_t[size];
    memcpy(buffToSave,c,size);

    buf.push_back(std::make_pair(buffToSave,size));
    fill ++;
    dataWriteEvent.notify();

    return true;
}

/** @brief read an element from the FiFo
 *
 * The function will return an element from the FiFo. The caller has to care about deleting the Element.
 *
 * @param c pointer to the element
 * @param size size of the element
 */
void BufferFiFo::read(uint8_t*& c, int &size)
{
    if(fill == 0)
    {
        wait(dataWriteEvent);
    }

    std::pair <uint8_t*,int>element = this->buf.front();
    c = element.first;
    size = element.second;
    fill --;
    this->buf.pop_front();

    dataReadEvent.notify();
}

/** @brief interface to @read(uint8_t*& c, int &size)
 *
 *
 * @param size size of the element
 *
 * return pointer to the element
 */
uint8_t* BufferFiFo::read(int &size)
{
    uint8_t* c;
    read(c, size);
    return c;
}

#undef MODULE_ID_STR
