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
 * @DemuxSplit.h This Module looks for the given Pids, and filter them.
 *
 *  It splits the ts packets according to their Pids to the given outputs (audio and video), and send an update to the STC
 *  if an pcr package occurs.
 *
 */

#ifndef DEMUXSPLIT_H_
#define DEMUXSPLIT_H_

#include <modules/elements/buffers/BufferFiFo.h>
#include "systemc.h"
#include "mpeg/ts.h"
#include "mpeg/pes.h"
#include "framework/Configuration.h"
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <memory>
#include <map>
#include "../buffers/BufferDecoder.h"

#define PES_BUFFER_SIZE 8000000 //8MB

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/demux/DemuxSplit"


SC_MODULE(DemuxSplit)
{
    sc_port<BufferFillInIf,1,SC_ZERO_OR_MORE_BOUND> in; /** the police is necesarry if the port gets "overwriten" by a new interface*/
    sc_out<int64_t> stcOut;
    sc_inout<bool> stcStarted;
    sc_port<BufferDecoderOutIf> videoOut;
    sc_port<BufferDecoderOutIf> audioOut;
    sc_out<bool> stcSendRequ;
    sc_in<int64_t> stcGet;
    sc_out<bool> stcSendRequOffset;
    sc_in<int64_t> stcGetOffset;


    int timeToPresentVideo;
    int timeToPresentAudio;

    int timeToPresentVideoIncludingStcOffset;
    int timeToPresentAudioIncludingStcOffset;

    int videoPid;
    int audioPid;
    int pcrPid;
    std::map<int,int> ccCounter;
    int bufferSize;

    int pesVideoBufferFill = 0;
    int pesVideoPacketSize = 0;/** just for logging **/

    int pesAudioBufferFill = 0;
    int pesAudioPacketSize = 0;/** just for logging **/

private:
    uint8_t* pesVideoBuffer;
    bool pesVideoBufferInit = false;

    uint8_t* pesAudioBuffer;
    bool pesAudioBufferInit = false;


    std::shared_ptr<CsvTrace> m_csvTrace;

    /** @brief function to parse Pids out for the configuration
     * 
     * @param s the rapidJson value
     * @param id a string to identify if its the video, audio or pcr pid (config sting)
     * 
     * @return a pid that should pass the dmx, to the different outputs
     * 
     */
    int parsePid(rapidjson::Value& s, string id)
    {
        int result;
        if (!s.HasMember(id.c_str()) || !s[id.c_str()].IsInt()) {
            std::string message;
            message += "Malformed configuration for \"";
            message += this->name();
            message += "\". ";
            message += id;
            message +=" is missing or no Int";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }
        result = s[id.c_str()].GetInt();
        ccCounter[result] = -1;

        return result;
    }

    /** @brief this function check the Countiuie Counter counter.
     * 
     * It maintains a map with the setter pids and is able to detect an
     * Error in the continue counter.
     * 
     * @param tsPacket the ts packet, that is to check
     * @param pid the pid of the ts packet.
     * 
     * @return false if the cc counter is constant and there is no payload.
     *
     */
    bool checkCorrectCcCounter(uint8_t* tsPacket, int pid)
    {
        if (ccCounter[pid] == -1)
        {
            ccCounter[pid] = ts_get_cc(tsPacket);
            return true;
        }
        else if (ts_get_cc(tsPacket) == ((ccCounter[pid] + 1) % 16))
        {
            ccCounter[pid] = ((ccCounter[pid] + 1) % 16);
            return true;
        }
        else if(ts_get_cc(tsPacket) == (ccCounter[pid] % 16))
        {
            //when no data present, ccCounter must not increase
            if(!ts_has_payload(tsPacket))
            {
                return true;
            }
            else
            {
                SC_REPORT_WARNING(MODULE_ID_STR,"Double Packet, skip");
                return false;
            }
        }
        else
        {
            std::string msg;
            msg += "continuity counter fail, Pid: ";
            msg += std::to_string(pid);
            msg += " expected ";
            msg += std::to_string((ccCounter[pid] + 1) % 16);
            msg += " got ";
            msg += std::to_string(ts_get_cc(tsPacket));

            SC_REPORT_WARNING(MODULE_ID_STR,msg.c_str());
            ccCounter[pid] = ts_get_cc(tsPacket);
            return true;
        }
    }

public:

    /** @brief loads the configuration out of the given .json file.
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

        this->videoPid = parsePid(s,"videoPid");
        this->audioPid = parsePid(s,"audioPid");
        this->pcrPid = parsePid(s,"pcrPid");

        if (!s.HasMember("trace") || !s["trace"].IsBool()) {
            std::string message;
            message += "Malformed configuration for \"";
            message += this->name();
            message += "\". \"trace\" is missing or no Bool. This Module will not been logged";
            SC_REPORT_WARNING(MODULE_ID_STR, message.c_str());
        } else {
            if (s["trace"].GetBool()) {
                m_csvTrace = std::make_shared<CsvTrace>(config.dir());
                m_csvTrace->delta_cycles(true);
                m_csvTrace->trace(this->timeToPresentVideo, std::string(this->name()).append(".timeToPresentVideo"), "time to present in 1/90e3s");
                m_csvTrace->trace(this->timeToPresentAudio, std::string(this->name()).append(".timeToPresentAudio"), "time to present in 1/90e3s");
                m_csvTrace->trace(this->timeToPresentVideoIncludingStcOffset, std::string(this->name()).append(".timeToPresentVideoIncludingStcOffset"), "time to present in 1/90e3s");
                m_csvTrace->trace(this->timeToPresentAudioIncludingStcOffset, std::string(this->name()).append(".timeToPresentAudioIncludingStcOffset"), "time to present in 1/90e3s");
                m_csvTrace->trace(this->pesVideoPacketSize, std::string(this->name()).append(".pesVideoPacketSize"), "size in bytes");
                m_csvTrace->trace(this->pesAudioPacketSize, std::string(this->name()).append(".pesAudioPacketSize"), "size in bytes");
            }
        }
    }
    
    
    /**@brief this function cares about filling and sending a video Buffer
     *
     * This function gets the TS packates and saves the payload until it gets a full PES packet.
     * It saves the pes payload together with the pts to the next Buffer
     *
     * @param tsPacket pointer to the 188 byte Transport stream packet
     *
     */
    void fillVideoESPacket(uint8_t* tsPacket) {
        if (!pesVideoBufferInit && !ts_get_unitstart(tsPacket))
        {
            return;
        }
        if(ts_get_unitstart(tsPacket))
        {
            if(pesVideoBufferInit)
            {

                if (pes_validate_header(pesVideoBuffer) && pes_has_pts(pesVideoBuffer) && pes_validate_pts(pesVideoBuffer))
                {

                    uint64_t pts = pes_get_pts(pesVideoBuffer);

                    uint8_t* pesPayloadStart = pes_payload(pesVideoBuffer);
                    this->pesVideoPacketSize = pesVideoBufferFill;
                    int pesPayloadSize = pesVideoBufferFill - (pesPayloadStart - pesVideoBuffer);

                    uint8_t* pesPayload = new uint8_t[pesPayloadSize];
                    memcpy(pesPayload, pesPayloadStart, pesPayloadSize);

                    stcSendRequ.write(true);
                    wait(stcGet.default_event());
                    int64_t stc = stcGet.read();
                    stcSendRequ.write(false);

                    stcSendRequOffset.write(true);
                    wait(stcGetOffset.default_event());
                    int64_t stcOffset = stcGetOffset.read();
                    stcSendRequOffset.write(false);


                    timeToPresentVideo = pts - stc;
                    timeToPresentVideoIncludingStcOffset = pts - stcOffset;

                    videoOut->write(pesPayload, pts, pesPayloadSize);
                }
                else
                {
                    SC_REPORT_WARNING(MODULE_ID_STR,"invalid pes packet Video");
                }
                delete[] pesVideoBuffer;
            }
            else
            {
                pesVideoBufferInit = true;
            }
            pesVideoBuffer = new uint8_t[PES_BUFFER_SIZE];
            pesVideoBufferFill = 0;
        }

        uint8_t* tsPayload = ts_payload(tsPacket);
        int tsPayloadSize = TS_SIZE - (tsPayload - tsPacket);

        memcpy(pesVideoBuffer+pesVideoBufferFill, tsPayload, tsPayloadSize);
        pesVideoBufferFill += tsPayloadSize;
    }

    /**@brief this function cares about filling and sending a audio Buffer
     *
     * This function gets the TS packates and saves the payload until it gets a full PES packet.
     * It saves the pes payload together with the pts to the next Buffer
     *
     * @param tsPacket pointer to the 188 byte Transport stream packet
     *
     */
    void fillAudioESPacket(uint8_t* tsPacket) {
        if (!pesAudioBufferInit && !ts_get_unitstart(tsPacket))
        {
            return;
        }
        if(ts_get_unitstart(tsPacket))
        {
            if(pesAudioBufferInit)
            {

                if (pes_validate_header(pesAudioBuffer) && pes_has_pts(pesAudioBuffer) && pes_validate_pts(pesAudioBuffer))
                {

                    uint64_t pts = pes_get_pts(pesAudioBuffer);

                    uint8_t* pesPayloadStart = pes_payload(pesAudioBuffer);
                    this->pesAudioPacketSize = pesAudioBufferFill;
                    int pesPayloadSize = pesAudioBufferFill - (pesPayloadStart - pesAudioBuffer);

                    uint8_t* pesPayload = new uint8_t[pesPayloadSize];
                    memcpy(pesPayload, pesPayloadStart, pesPayloadSize);

                    stcSendRequ.write(true);
                    wait(stcGet.default_event());
                    int64_t stc = stcGet.read();
                    stcSendRequ.write(false);

                    stcSendRequOffset.write(true);
                    wait(stcGetOffset.default_event());
                    int64_t stcOffset = stcGetOffset.read();
                    stcSendRequOffset.write(false);

                    timeToPresentAudio = pts - stc;
                    timeToPresentAudioIncludingStcOffset = pts - stcOffset;

                    audioOut->write(pesPayload, pts, pesPayloadSize);
                }
                else
                {
                    SC_REPORT_WARNING(MODULE_ID_STR,"invalid pes packet Audio");
                }
                delete[] pesAudioBuffer;
            }
            else
            {
                pesAudioBufferInit = true;
            }
            pesAudioBuffer = new uint8_t[PES_BUFFER_SIZE];
            pesAudioBufferFill = 0;
        }

        uint8_t* tsPayload = ts_payload(tsPacket);
        int tsPayloadSize = TS_SIZE - (tsPayload - tsPacket);

        memcpy(pesAudioBuffer+pesAudioBufferFill, tsPayload, tsPayloadSize);
        pesAudioBufferFill += tsPayloadSize;
    }



    /** @brief this function models a demux.
     * 
     * This demux gets its data from the tuner, and feed them either to the video pipeline, to the audio pipline or
     * to the STC.
     */
    void demuxPoc() {
        int64_t pcr;
        uint8_t* tsBuffer;
        uint8_t* tsPacket;

        while (true) {
            getBuffer(tsBuffer, bufferSize);

            for (int i = 0; (i+1)*TS_SIZE <= bufferSize; i++)
            {

                tsPacket = tsBuffer + i*TS_SIZE;

                int pid = ts_get_pid(tsPacket);

                /* ts packet in Pids?
                 * If not skip the package. It will be deleted along with the buffer at the end (in->releaseBuffer(tsBuffer)).
                 */
                if ((pid == this->pcrPid) ||
                    (pid == this->videoPid) ||
                    (pid == this->audioPid))
                {
                    //yes check cc
                    if(!checkCorrectCcCounter(tsPacket,pid))
                    {
                        //cc error, skip package
                        continue;
                    }
                }

                if (pid == this->pcrPid)
                {
                    if (ts_has_adaptation(tsPacket) && (ts_get_adaptation(tsPacket) != 0) && tsaf_has_pcr(tsPacket)) {
                        pcr =  tsaf_get_pcr(tsPacket) * 300 + tsaf_get_pcrext(tsPacket);
                        stcOut.write(pcr);
                        if(!stcStarted.read())
                        {
                            stcStarted.write(true);
                        }
                    }
                }
                if (pid == this->videoPid)
                {
                    this->fillVideoESPacket(tsPacket);
                }
                else if (pid == this->audioPid)
                {
                    this->fillAudioESPacket(tsPacket);
                }
            }

            releaseBuffer(tsBuffer);
        }
    }
    
    /** @brief this function cares about geting a buffer for the Demux to deal with
     *
     *  @param buffer pointer to a Buffer
     *  @param bufferSize size of the Buffer
     */
    virtual void getBuffer(uint8_t*& buffer, int& bufferSize)
    {
        in->read(buffer);
        bufferSize = TS_SIZE;
    }

    /** @brief this function cares about releasing a buffer after the Demux dealt with it.
     *
     *  @param buffer pointer to the Buffer to release
     */
    virtual void releaseBuffer(uint8_t*& buffer)
    {
        delete[] buffer;
        buffer = NULL;
    }

    /** @brief systemC constructor. 
     *
     *     
     */
    SC_CTOR(DemuxSplit):
        in("in")
    {
        this->loadConfig();
        SC_THREAD(demuxPoc);
    }


};
#undef MODULE_ID_STR
#undef PES_BUFFER_SIZE

#endif //DEMUXSPLIT_H_



