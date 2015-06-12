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
 * This model simulates the behaviour of a DVB tuner. It reads 188 Byte packages from a TS file, and feed them with a constant rate (bitrate)
 * into the next Model, a Buffer.
 * It looks for a TS Sync Byte, before it sends the package.
 */

#ifndef READTS_H_
#define READTS_H_

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

SC_MODULE(TunerDVB)
{
public:
    sc_port<BufferFillOutIf> out;

private:
    std::shared_ptr<CsvTrace> m_csvTrace;
    std::string filename;
    double bitRate;
    double readTimeOut;

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

        if (!s.HasMember("bitRate") || !s["bitRate"].IsDouble()) {
            std::string message;
            message += "Malformed configuration for: \"";
            message += this->name();
            message += "\". \"bitRate\" is missing or no Double";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }

        this->bitRate = s["bitRate"].GetDouble();
        this->readTimeOut = 188 / (this->bitRate/8);


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
        ifstream file;
        file.open(this->filename, ifstream::in | ifstream::binary );

        while (true) {
            tsPacket = new char[188];
            file.read(tsPacket, 188);

            if (!file) {
                std::string message;
                message += "file Error (Maybe end of file reached) for: \"";
                message += this->name();
                message += "\".";
                SC_REPORT_WARNING(MODULE_ID_STR, message.c_str());
                break;
            }

            while (!ts_validate((uint8_t*)tsPacket))
            {
                if (search == 0)
                {
                    SC_REPORT_WARNING(MODULE_ID_STR,"invalid tsPacket, trying to find sync byte");
                }
                file.seekg(-187, ios_base::cur);
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

            out->write((uint8_t*) tsPacket);
            wait(this->readTimeOut, SC_SEC);
        }

        file.close();
    }

    SC_CTOR(TunerDVB) {
        this->loadConfig();
        SC_THREAD(read);
    }
};
#undef MODULE_ID_STR
#endif //READTS_H_
