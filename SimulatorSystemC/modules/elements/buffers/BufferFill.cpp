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
 * @BufferFill.h Simulate a Buffer, that just empties after it is fully filled.
 *
 */

#include <modules/elements/buffers/BufferFill.h>
#include "framework/Configuration.h"
// constructor

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/buffers/BufferFill"

/** @brief constructor
 *
 */
BufferFill::BufferFill(const char* my_name)
    : sc_prim_channel(my_name)
{
    this->loadConfig();
    buf = new uint8_t*[this->size];
    reset();
}

/** @brief load config from a .json file.
 *
 */
void BufferFill::loadConfig()
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
            m_csvTrace->trace(this->rd, std::string(this->name()).append(".rd"), "readPointer in elements");
        }
    }
}

BufferFill::~BufferFill()                   //destructor
{
    delete [] buf;
}

/** @brief write c to the buffer
 *
 * When reading is in progress will block, until buffer is Empty
 *
 * @param c pointer witch will be stored.
 *
 */
void BufferFill::write(uint8_t* c)        // blocking write
{
    if (fill == size) {
        wait(dataEmptyEvent);
    }

    buf[fill] = c;
    fill++;

    if (fill == size) {
        dataFullEvent.notify(SC_ZERO_TIME);
    }

}

/** @brief reset the buffer
 */
void BufferFill::reset()
{
    fill = 0;
    rd = 0;
    readState = false;
}

/** @brief wait till buffer is full. Then allow read until buffer is empty again.
 *
 * @param c pointer to an stored element
 *
 */
void BufferFill::read(uint8_t*& c)        // blocking read
{
    if (!(fill == size)) {
        wait(dataFullEvent);
    }

    c = buf[rd];
    rd++;
    //force delta cycle
    wait(SC_ZERO_TIME);

    if (rd == fill) {
        reset();
        dataEmptyEvent.notify(SC_ZERO_TIME);
    }
}

/** @brief wait till buffer is full. Then allow read until buffer is empty again.
 *
 * @return pointer to an stored element
 *
 */
uint8_t* BufferFill::read()                // shortcut read function
{
    uint8_t* c;
    read(c);
    return c;
}
#undef MODULE_ID_STR

