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

#include "BufferDecoder.h"

#include "framework/Configuration.h"

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/buffers/BufferVideoDecoder"

BufferDecoder::BufferDecoder(const char* name)
    : sc_prim_channel(name)
{
    this->loadConfig();
    this->reset();
}

/** @brief this function loads the confguration
 *
 */
void BufferDecoder::loadConfig()
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

    this->m_size = s["size"].GetInt();

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
            m_csvTrace->trace(this->fill, std::string(this->name()).append(".fill"), "fill in Bytes");
        }
    }
}

BufferDecoder::~BufferDecoder() {
}

/** @brief reset the Buffer
 *
 */
void BufferDecoder::reset()
{
    fill = 0;
    m_bitstreamBuffer.empty();
    m_ptsBuffer.empty();
}

/** @brief write a element in the buffer
 *
 * This function saves an element in the Buffer. It will save the pts and the size as well.
 * Size is used to calculate the filling of the Buffer.
 *
 * @param[in] buffer element to Store (pes_payload)
 * @param[in] pts presentation time stamp according to the buffer element
 * @param[in] size is the size of the buffer
 *
 */
void BufferDecoder::write(uint8_t* buffer, int64_t pts, int size)
{
    /**
     * just because some data are removed, this migth not necesarry mean, that there is now enogth space
     */
    while (this->fill+size > this->m_size)
    {
        wait(this->m_dataReadEvent);
    }

    this->m_bitstreamBuffer.push_back(std::make_pair(buffer, size));
    this->m_ptsBuffer.push_back(pts);
    this->fill += size;

    this->m_dataWriteEvent.notify();
}

/** @brief get a element from the buffer
 *
 * This function read an element from the Buffer and removes it.
 *
 * @param[out] buffer reference to the buffer to fill.
 * @param[out] pts reference to an int64_t witch will holt the pts
 * @param[out] size of the buffer retrurned
 *
 */
void BufferDecoder::read(uint8_t*& buffer, int64_t& pts, int& size)
{
    if(m_bitstreamBuffer.empty())
    {
        wait(this->m_dataWriteEvent);
    }
    std::pair <uint8_t*, int>element = this->m_bitstreamBuffer.front();
    buffer = element.first;
    size = element.second;
    pts = this->m_ptsBuffer.front();

    this->m_bitstreamBuffer.pop_front();
    this->m_ptsBuffer.pop_front();
    this->fill -= size;

    this->m_dataReadEvent.notify();
}

/** @brief return the fullness of the buffer
 *
 * @retrun fullness of Buffer in percent
 *
 */
double BufferDecoder::fillPercent()
{
    return (double)fill * 100 / double(m_size);
}

/** @brief return the size of the buffer
 *
 * @return the size of the buffer
 */
int BufferDecoder::getSize()
{
    return this->m_size;
}
#undef MODULE_ID_STR
