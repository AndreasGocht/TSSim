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
 * @file This file Simulates the decoding of video frames.
 *
 * The Decoder receives the Video Frames (PES payload) together with the PTS value.
 *
 * If the Video is an MPEG video, the Decoder searches for MPEG frames, and Calculate a PTS for frames in the given BLOB.
 * If the stream is any other format, it will push the BLOB togehter with the PTS to a picture Buffer.
 *
 */

#ifndef MODULES_ELEMENTS_PESDECODER_VIDEODECODER_H_
#define MODULES_ELEMENTS_PESDECODER_VIDEODECODER_H_

#include <modules/elements/buffers/BufferPicture.h>
#include "systemc.h"
#include <stdint.h>
#include <cstring> // memcpy

#include "../buffers/BufferDecoder.h"
#include "framework/Configuration.h"
#include "framework/CsvTrace.h"

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/pesDecoder/VideoDecoder"
#define STC_COUNT_PER_SECOND 90e3
#define BITSTREAM_MPEG_VIDEO "13818-2 video (MPEG-2)"

#define PICT_BUFFER_SIZE 4000000 //4MB

SC_MODULE(VideoDecoder)
{
    sc_port<BufferDecoderInIf> esPacketIn;
    sc_port<BufferPictureOutIf> pictureOut;
    sc_out<bool> stcSendRequ;
    sc_in<int64_t> stcGet;

    sc_out<bool> stcSendRequOffset;
    sc_in<int64_t> stcGetOffset;

    string videoTyp;


    int timeToPresent;
    int timeToPresentIncludingStcOffset;

    int framesPerSecond = 0;
    int framesPerSecondCount = 0;
    sc_time framesPerSecondLastTime;
    int framesPerMinute = 0;
    int framesPerMinuteCount = 0;
    sc_time framesPerMinuteLastTime;

    double framerate = 0;
    int countPict = 0;

    double decodingTime = 0;

    std::shared_ptr<CsvTrace> m_csvTrace;

    /** @brief load the configuration
     *
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

        if (!s.HasMember("videoTyp") || !s["videoTyp"].IsString())
        {
            std::string message;
            message += "Malformed configuration of \"";
            message += this->name();
            message += "\". \"videoTyp\" is missing or no String.";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }

        this->videoTyp = s["videoTyp"].GetString();

        if (!s.HasMember("decodingTime") || !s["decodingTime"].IsDouble())
        {
            std::string message;
            message += "Malformed configuration of \"";
            message += this->name();
            message += "\". \"decodingTime\" is missing or no Double.";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }

        this->decodingTime = s["decodingTime"].GetDouble();


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
                m_csvTrace->trace(this->countPict, std::string(this->name()).append(".countPict"), "Pictures per Frame");
                m_csvTrace->trace(this->framerate, std::string(this->name()).append(".framerate"), "framerate for sequence");
            }
        }
    }

    /** @brief the parsing and "decoding" process.
     *
     * It will parse the given pes_payloads for pictures and push them to the next buffer.
     *
     */
    void process() {
        uint8_t* esPacket;
        int64_t pts;
        int64_t key;
        int size;
        int64_t stc;
        int64_t stcOffset;


        while (true) {
            esPacketIn->read(esPacket, pts, size);

            wait(decodingTime,SC_SEC);

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

            if(videoTyp == BITSTREAM_MPEG_VIDEO)
            {
                countPict = 0;
                int pictStart = 0;
                uint8_t* pictBuff;
                int pictBuffSize = 0;

                for(int i = 0; i < size-3;i++)
                {
                    if((esPacket[i+0] == 0x00) &&
                       (esPacket[i+1] == 0x00) &&
                       (esPacket[i+2] == 0x01) &&
                       (esPacket[i+3] == 0xb3))
                    {
                        //sequence header found, look for frame rate:
                        switch (esPacket[7] & 0x0f)
                        {
                            case 0b0001:
                                framerate = 24/1.001;
                                break;
                            case 0b0010:
                                framerate = 24;
                                break;
                            case 0b0011:
                                framerate = 25;
                                break;
                            case 0b0100:
                                framerate = 30/1.001;
                                break;
                            case 0b0101:
                                framerate = 30;
                                break;
                            case 0b0110:
                                framerate = 50;
                                break;
                            case 0b0111:
                                framerate = 60/1.001;
                                break;
                            case 0b1000:
                                framerate = 60;
                                break;
                            default:
                                framerate = 25;
                                SC_REPORT_WARNING(MODULE_ID_STR,"could't detect framerate, using 25 Hz");
                        }
                    }

                    if((esPacket[i+0] == 0x00) &&
                       (esPacket[i+1] == 0x00) &&
                       (esPacket[i+2] == 0x01) &&
                       (esPacket[i+3] == 0x00))
                    {
                        if (pictStart != 0)
                        {
                            pictBuff = new uint8_t[PICT_BUFFER_SIZE];
                            pictBuffSize = i - pictStart;
                            std::memcpy(pictBuff, esPacket + pictStart, pictBuffSize);
                            key = pictureOut->write(pictBuff, pts + (int)(((1.0/framerate) * countPict) * STC_COUNT_PER_SECOND), pictBuffSize);
                            pictureOut->finished(std::list<int64_t>(1, key));
                            countPict ++;
                        }
                        pictStart = i;

                    }
                }
                // copy the last picture
                if (pictStart != 0)
                {
                    pictBuff = new uint8_t[PICT_BUFFER_SIZE];
                    pictBuffSize = size - pictStart;
                    std::memcpy(pictBuff, esPacket + pictStart, pictBuffSize);
                    key = pictureOut->write(pictBuff, pts + (int)(((1.0/framerate) * countPict) * STC_COUNT_PER_SECOND), pictBuffSize);
                    pictureOut->finished(std::list<int64_t>(1, key));
                    delete[] esPacket;
                }
                else
                {
                    // there seem to bee al lot of streams that have no picture header, so just push the pes packet
                    key = pictureOut->write(esPacket, pts, size);
                    pictureOut->finished(std::list<int64_t>(1, key));
                }
            }
            else
            {
                //fallback for other videos that are not mpeg
                key = pictureOut->write(esPacket, pts, size);
                pictureOut->finished(std::list<int64_t>(1, key));

            }
        
            if((sc_time_stamp() - framesPerSecondLastTime) > sc_time(1, SC_SEC))
            {
                framesPerSecond = framesPerSecondCount;
                framesPerSecondCount = 0;
                framesPerSecondLastTime = sc_time_stamp();
            }
            framesPerSecondCount ++;
            if((sc_time_stamp() - framesPerMinuteLastTime) > sc_time(60, SC_SEC))
            {
                framesPerMinute = framesPerMinuteCount;
                framesPerMinuteCount = 0;
                framesPerMinuteLastTime = sc_time_stamp();
            }
            framesPerMinuteCount ++;
        }
    }

    SC_CTOR(VideoDecoder) {
        loadConfig();
        SC_THREAD(process);
    }

    ~VideoDecoder()
    {
    }
};
#undef MODULE_ID_STR

#endif /*MODULES_ELEMENTS_PESDECODER_VIDEODECODER_H_*/
