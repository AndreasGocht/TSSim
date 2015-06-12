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
 * @file This file holds the implementation for the outPut element
 *
 * It tires to get a frame at a given Framerate from the sync element.
 *
 */

#ifndef OUTPUT_H_
#define OUTPUT_H_

#include "systemc.h"
#include "framework/Configuration.h"
#include "rapidjson/document.h"
#include "framework/CsvTrace.h"
#include <string>

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/OutPut"


SC_MODULE(OutPut)
{
public:
    sc_out<bool> frameRequest;
    sc_in<uint8_t*> frameIn;

    bool displayFrame = false;
    double frameTimeOut = 0;
    double framerate = 0;
private:
    std::shared_ptr<CsvTrace> m_csvTrace;
    bool m_firstFrameShowed = false;
    bool m_stutterLoged = false;

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

        if (!s.HasMember("framerate") || !s["framerate"].IsDouble()) {
            std::string message;
            message += "Malformed configuration of \"";
            message += this->name();
            message += "\". \"framerate\" is missing or no Double.";
            SC_REPORT_FATAL(MODULE_ID_STR , message.c_str());
        }

        this->framerate = s["framerate"].GetDouble();
        this->frameTimeOut = 1/framerate;

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
                m_csvTrace->trace(displayFrame, std::string(this->name()).append(".displayFrame"), "Frame to display in bool (1 = ther is a frame; 0 = there is no frame)");
            }
        }
    }

    /** @brief this function implements the OutPut element
     *
     *
     * It try to get a picture on the given Framerate. It will log if it was able to display a Picture, or not.
     *
     */
    void process()
    {
        uint8_t* frame = NULL;
        while(true)
        {
            frameRequest.write(true);
            wait(frameIn.default_event());
            frame = frameIn.read();
            if(frame == NULL)
            {
                if (m_firstFrameShowed && !m_stutterLoged)
                {
                    m_stutterLoged = true;
                    std::string message;
                    message += this->name();
                    message += ": stutter occurred";
                    SC_REPORT_WARNING(MODULE_ID_STR, message.c_str());
                }
                displayFrame = false;
            }
            else
            {
                if (!m_firstFrameShowed){
                    m_firstFrameShowed = true;
                }
                displayFrame = true;
                delete[] frame;
            }
            frameRequest.write(false);

            wait(frameTimeOut, SC_SEC);
        }
    }

    SC_CTOR(OutPut) {
        loadConfig();
        SC_THREAD(process);
    }

};

#undef MODULE_ID_STR
#endif /* OUTPUT_H_ */


