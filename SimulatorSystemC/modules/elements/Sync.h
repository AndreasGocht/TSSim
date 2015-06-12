/*
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
 * @file This file holds the implementation for a sync element
 *
 */

#ifndef SYNC_H_
#define SYNC_H_


#include <modules/elements/buffers/BufferPicture.h>
#include "systemc.h"
#include "framework/Configuration.h"
#include "rapidjson/document.h"
#include "framework/CsvTrace.h"
#include <string>

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/Sync"


SC_MODULE(Sync)
{
public:
    sc_port<BufferPictureInIf> frameIn;
    sc_in<bool> frameRequest;
    sc_out<uint8_t*> frameOut;

    sc_out<bool> stcSendRequ;
    sc_in<int64_t> stcGet;


private:
    std::shared_ptr<CsvTrace> m_csvTrace;

public:
    /** @brief loads the configuration out of a given .json file.
    */
    void loadConfig() {
        Configuration& config = Configuration::getInstance();

        if (!config.HasMember(this->name())) {
            std::string message;
            message += "No Configuration found for: \"";
            message += this->name();
            message += "\"";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }

        rapidjson::Value& s = config[this->name()];

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
                /*nothing to Trace
                 * use
                 * m_csvTrace->trace(variable,file ending,ylable)
                 * to trace a property
                 * */
            }
        }
    }

    /** @brief this function implements the Sync element
     *
     *
     * It retrieves the actual stc value, and tries to get a picture from the display Buffer.
     *
     */
    void process()
    {
        int64_t stc = 0;
        uint8_t* frame = NULL;
        int size = 0;
        while(true)
        {
            wait(frameRequest.default_event());
            if (frameRequest.read())
            {
                stcSendRequ.write(true);
                wait(stcGet.default_event());
                stc = stcGet.read();
                stcSendRequ.write(false);
                frameIn->nbread(frame, stc, size);
                frameOut.write(frame);
            }
        }
    }

    SC_CTOR(Sync) {
        this->loadConfig();
        SC_THREAD(process);
    }

};
#undef MODULE_ID_STR
#endif /* SYNC_H_ */

