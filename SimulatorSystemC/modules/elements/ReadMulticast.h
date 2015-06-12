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
 * this element takes a multicast and an aux file, used by multicat. It uses the same way like multicat of
 * determining at witch time a package shlud be send.
 */

#ifndef READMULTICAST_H_
#define READMULTICAST_H_

#include <modules/elements/buffers/BufferFill.h>
#include "systemc.h"
#include "framework/Configuration.h"
#include "rapidjson/document.h"
#include "framework/CsvTrace.h"
#include "mpeg/ts.h"
#include <string>
#include <fstream>
#include <memory>

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/ReadTs"

SC_MODULE(ReadMulticast)
{
public:
    sc_port<BufferFillOutIf> out;
    std::string filename;
    std::string filenameAux;

private:
    std::shared_ptr<CsvTrace> m_csvTrace;
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

        if (!s.HasMember("filename") || !s["filename"].IsString()) {
            std::string message;
            message += "Malformed configuration for: \"";
            message += this->name();
            message += "\". \"filename\" is missing or no String";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }

        this->filename = s["filename"].GetString();

        if (!s.HasMember("filenameAux") || !s["filenameAux"].IsString()) {
            std::string message;
            message += "Malformed configuration for: \"";
            message += this->name();
            message += "\". \"filenameAux\" is missing or no String";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }

        this->filenameAux = s["filenameAux"].GetString();

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
                /*nothing to Trace
                 * use
                 * m_csvTrace->trace(variable,file ending,ylable)
                 * to trace a property
                 * */
            }
        }
    }

    void read() {
        int search = 0;
        char* tsPacket;
        char pAux[8];
        ifstream file;
        ifstream fileAux;
        uint64_t auxStc;
        double timeToSleep;
        file.open(this->filename, ifstream::in | ifstream::binary );
        fileAux.open(this->filenameAux, ifstream::in | ifstream::binary );


        while (true) {
            tsPacket = new char[188];
            file.read(tsPacket, 188);
            fileAux.read(pAux,8);

            if (!file) {
                std::string message;
                message += "file Error (Maybe end of file reached) for: \"";
                message += this->name();
                message += "\".";
                SC_REPORT_WARNING(MODULE_ID_STR, message.c_str());
                break;
            }

            if (!fileAux) {
                std::string message;
                message += "fileAux Error (Maybe end of file reached) for: \"";
                message += this->name();
                message += "\".";
                SC_REPORT_WARNING(MODULE_ID_STR, message.c_str());
                break;
            }

            /*
             * compare to mulitcat util.h FromSTC()
             */
            auxStc = ((uint64_t)pAux[0] << 56)
                 | ((uint64_t)pAux[1] << 48)
                 | ((uint64_t)pAux[2] << 40)
                 | ((uint64_t)pAux[3] << 32)
                 | ((uint64_t)pAux[4] << 24)
                 | ((uint64_t)pAux[5] << 16)
                 | ((uint64_t)pAux[6] << 8)
                 | ((uint64_t)pAux[7] << 0);
            timeToSleep = auxStc/27e6;

            while (!ts_validate((uint8_t*)tsPacket))
            {
                if (search == 0)
                {
                    SC_REPORT_WARNING(MODULE_ID_STR,"invalid tsPacket, trying to find sync byte");
                }
                file.seekg(-187,ios_base::cur);
                file.read(tsPacket, 188);
                search ++;
            }
            if (search != 0)
            {
                std::string message;
                message += "sync byte found ";
                message += std::to_string(search);
                message += " bytes Later.";
                SC_REPORT_WARNING(MODULE_ID_STR, message.c_str());
                search = 0;
            }

            out->write((uint8_t*)tsPacket);
            /*
             * multicat sleeps before sending. We sleep after. So we don't have to care about the first package (witch has no delay).
             * We could do this, because reading from file, dons't matter in the simulation time, while multicat has to care about the reading
             * time.
             */
            wait(timeToSleep,SC_SEC);
        }

        file.close();
    }

    SC_CTOR(ReadMulticast) {
        this->loadConfig();
        SC_THREAD(read);
    }
};

#undef MODULE_ID_STR
#endif //READMULTICAST_H_
