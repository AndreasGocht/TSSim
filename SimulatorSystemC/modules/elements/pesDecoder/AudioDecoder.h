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
 * @file This file Simulates the decoding of audio frames.
 *
 * The Decoder receives the Audio TS packet. It collects a whole PES frame, decode the PTS,
 * and push the PES packet together with the PTS to an output Buffer.
 *
 */

#ifndef MODULES_ELEMENTS_PESDECODER_AUDIODECODER_H_
#define MODULES_ELEMENTS_PESDECODER_AUDIODECODER_H_

#include <modules/elements/buffers/BufferFiFo.h>
#include <modules/elements/buffers/BufferPicture.h>
#include "systemc.h"
#include "mpeg/ts.h"
#include "mpeg/pes.h"
#include <stdint.h>
#include <cstring> // memcpy
#include "framework/Configuration.h"
#include "framework/CsvTrace.h"

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/pesDecoder/AudioDecoder"

#define STC_COUNT_PER_SECOND 90e3

#define PES_BUFFER_SIZE 4000000 //4MB

SC_MODULE(AudioDecoder)
{
    sc_port<BufferDecoderInIf> esPacketIn;
    sc_port<BufferPictureOutIf> audioOut;
    sc_out<bool> stcSendRequ;
    sc_in<int64_t> stcGet;

    sc_out<bool> stcSendRequOffset;
    sc_in<int64_t> stcGetOffset;


    int timeToPresent;
    int timeToPresentIncludingStcOffset;
    int framesPerSecond = 0;
    int framesPerSecondCount = 0;
    sc_time framesPerSecondLastTime;
    int framesPerMinute = 0;
    int framesPerMinuteCount = 0;
    sc_time framesPerMinuteLastTime;

    std::shared_ptr<CsvTrace> m_csvTrace;



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
            SC_REPORT_WARNING(MODULE_ID_STR, message.c_str());
        } else {
            if (s["trace"].GetBool()) {
                m_csvTrace = std::make_shared<CsvTrace>(config.dir());
                m_csvTrace->delta_cycles(true);
                m_csvTrace->trace(this->timeToPresent, std::string(this->name()).append(".timeToPresent"), "time to present in 1/90e3s");
                m_csvTrace->trace(this->timeToPresentIncludingStcOffset, std::string(this->name()).append(".timeToPresentIncludingStcOffset"), "time to present in 1/90e3s");
                m_csvTrace->trace(this->framesPerSecond, std::string(this->name()).append(".framesPerSecond"), "pesPackets per second");
                m_csvTrace->trace(this->framesPerMinute, std::string(this->name()).append(".framesPerMinute"), "pesPackets per minute");
            }
        }
    }

    /** @brief the parsing and "decoding" process.
     *
     * It will push the pes data to the next buffer
     *
     */
    void process() {
        uint8_t* esPacket;
        int64_t pts;
        int64_t stc;
        int64_t stcOffset;
        int size;
        int64_t key;

        while (true) {
            esPacketIn->read(esPacket, pts, size);

            stcSendRequ.write(true);
            wait(stcGet.default_event());
            stc = stcGet.read();
            stcSendRequ.write(false);

            stcSendRequOffset.write(true);
            wait(stcGetOffset.default_event());
            stcOffset = stcGetOffset.read();
            stcSendRequOffset.write(false);

            timeToPresent = pts - stc;
            timeToPresentIncludingStcOffset = pts - stcOffset;


            key = audioOut->write(esPacket, pts, size);
            audioOut->finished(std::list<int64_t>(1, key));

            if((sc_time_stamp()-framesPerSecondLastTime)>sc_time(1,SC_SEC))
            {
                framesPerSecond = framesPerSecondCount;
                framesPerSecondCount = 0;
                framesPerSecondLastTime = sc_time_stamp();
            }
            framesPerSecondCount ++;
            if((sc_time_stamp()-framesPerMinuteLastTime)>sc_time(60,SC_SEC))
            {
                framesPerMinute = framesPerMinuteCount;
                framesPerMinuteCount = 0;
                framesPerMinuteLastTime = sc_time_stamp();
            }
            framesPerMinuteCount ++;
        }
    }
    SC_CTOR(AudioDecoder) {
        loadConfig();
        SC_THREAD(process);
    }

    ~AudioDecoder()
    {
    }
};
#undef MODULE_ID_STR

#endif
