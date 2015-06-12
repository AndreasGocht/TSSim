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
 */

#ifndef STC_STCOFFSET_H_
#define STC_STCOFFSET_H_

#include "systemc.h"
#include <cmath>
#include <vector>
#include <stdint.h>
#include "framework/CsvTrace.h"
#include "framework/Configuration.h"
#define MODULE_ID_STR "/digisoft/simulator/modules/elements/stc/StcPll"

SC_MODULE(StcOffset)
{
    sc_in<bool> stcRequestFromModule;
    sc_out<bool> stcRequestToStc;

    sc_in<int64_t> stcFromStc;
    sc_out<int64_t> stcToModule;

    std::shared_ptr<CsvTrace> m_csvTrace;
    int64_t offset = 0;

    int64_t stc = 0;
    int64_t stcOut = 0;
    int64_t oldStc = 0;
    int64_t warparoundOffset = 0;
    int64_t warparoundStc = 0;


public:
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


        if (!s.HasMember("offset") || !s["offset"].IsInt64()) {
            std::string message;
            message += "Malformed configuration of \"";
            message += this->name();
            message += "\". \"offset\" is missing or no Int64";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }

        offset = s["offset"].GetInt();

        if (!s.HasMember("trace") || !s["trace"].IsBool()) {
            std::string message;
            message += "Malformed configuration of \"";
            message += this->name();
            message += "\". \"trace\" is missing or no Bool. This Module will not been logged";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }

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
                m_csvTrace->trace(stc,std::string(this->name()).append(".inputStc"),"input in STC counts");
                m_csvTrace->trace(stcOut,std::string(this->name()).append(".outpurStc"),"output in STC counts");
            }
        }

    }

    void getStc(){
        bool stcWarparound = false;
        while (true) {
            wait(stcRequestFromModule.default_event());

            if (stcRequestFromModule.read()) {
                stcRequestToStc.write(true);
                wait(stcFromStc.default_event());
                oldStc = stc;
                stc = stcFromStc.read();
                if (stc < oldStc)
                {
                    // stc warparound
                    stcWarparound = true;
                    warparoundOffset = oldStc - stc;
                    warparoundStc = oldStc;
                }
                if (!stcWarparound)
                {
                    stcOut = stc-offset;
                }
                else
                {
                    if ((warparoundStc) < (stc-offset+warparoundOffset))
                    {
                        stcWarparound = false;
                        warparoundOffset = 0;
                        warparoundStc = 0;
                        stcOut = stc-offset;
                    }
                    else
                    {

                        stcOut = stc-offset+warparoundOffset;
                    }

                }
                // before first pcr the stc values are to small
                if (stcOut > 0)
                {
                    stcToModule.write(stcOut);
                }
                else
                {
                    stcToModule.write(0);
                }

                stcRequestToStc.write(false);
            }

        }
    }

    SC_CTOR(StcOffset) {
        loadConfig();
        SC_THREAD(getStc);
    }
};



#undef MODULE_ID_STR

#endif /* STC_STCOFFSET_H_ */
