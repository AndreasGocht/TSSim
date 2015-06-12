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
 * @ModelBasic.h This Model models a normal video pipeline.
 *
 */
#ifndef MODULEBASIC_H_
#define MODULEBASIC_H_

#include <modules/elements/buffers/BufferFiFo.h>
#include <modules/elements/buffers/BufferFill.h>
#include <modules/elements/buffers/BufferPicture.h>
#include <modules/elements/buffers/BufferDecoder.h>
#include <modules/elements/demux/DemuxSplit.h>
#include <modules/elements/OutPut.h>
#include <modules/elements/pesDecoder/AudioDecoder.h>
#include <modules/elements/pesDecoder/VideoDecoder.h>
#include <modules/elements/stc/Stc.h>
#include <modules/elements/stc/StcOffset.h>
#include <modules/elements/Sync.h>
#include <modules/elements/TunerDVB.h>

#include "systemc.h"



SC_MODULE(ModelBasic)
{
    TunerDVB read;
    BufferFill demuxInBuffer;
    DemuxSplit demux;
    Stc stc;
    StcOffset stcOffset;
    BufferDecoder videoDecoderBuffer;
    VideoDecoder videoDecoder;
    BufferPicture pictureBuffer;
    Sync syncVideo;
    OutPut outPutVideo;
    BufferDecoder audioDecoderBuffer;
    AudioDecoder audioDecoder;
    BufferPicture audioBuffer;
    Sync syncAudio;
    OutPut outPutAudio;

    sc_buffer<int64_t> stcPcrChan;
    sc_buffer<int64_t> stcStcChan;
    sc_buffer<bool> stcRequ;
    sc_buffer<int64_t> stcStcOffsetChan;
    sc_buffer<bool> stcOffsetRequ;
    sc_buffer<uint8_t*> outPutVideoGet;
    sc_buffer<bool> outPutVideoRequ;
    sc_buffer<uint8_t*> outPutAudioGet;
    sc_buffer<bool> outPutAudioRequ;

    sc_signal<bool> stcStarted;



    SC_CTOR(ModelBasic)
        :read("read")
        ,demuxInBuffer("demuxInBuffer")
        ,demux("demux")
        ,stc("stc")
        ,stcOffset("stcOffset")
        ,videoDecoderBuffer("videoDecoderBuffer")
        ,videoDecoder("videoDecoder")
        ,pictureBuffer("pictureBuffer")
        ,syncVideo("syncVideo")
        ,outPutVideo("outPutVideo")
        ,audioDecoderBuffer("audioDecoderBuffer")
        ,audioDecoder("audioDecoder")
        ,audioBuffer("audioBuffer")
        ,syncAudio("syncAudio")
        ,outPutAudio("outPutAudio")
        // systemc chans
        ,stcPcrChan("stcPcrChan")
        ,stcStcChan("stcStcChan")
        ,stcRequ("stcRequ")
        ,stcStcOffsetChan("stcStcOffsetChan")
        ,stcOffsetRequ("stcOffsetRequ")
        ,outPutVideoGet("outPutVideoGet")
        ,outPutVideoRequ("outPutVideoRequ")
        ,outPutAudioGet("outPutAudioGet")
        ,outPutAudioRequ("outPutAudioRequ")
        ,stcStarted("stcStarted")
    {
        //connect Modules
        //read-->demux
        read.out(demuxInBuffer);
        demux.in(demuxInBuffer);
        //demux --> pesDecoderVideo:
        demux.videoOut(videoDecoderBuffer);
        videoDecoder.esPacketIn(videoDecoderBuffer);
        //demux --> pesDecoderAudio:
        demux.audioOut(audioDecoderBuffer);
        audioDecoder.esPacketIn(audioDecoderBuffer);
        //demux --> stc
        demux.stcOut(stcPcrChan);
        stc.pcrIn(stcPcrChan);
        demux.stcStarted(stcStarted);
        stc.startStc(stcStarted);
        //stc <-- stc offset (request)
        //    <-- pesDecoderVideo (request)
        //    <-- pesDecoderAudio (request)
        //    <-- demux (request)
        stc.stcRequest(stcRequ);
        stcOffset.stcRequestToStc(stcRequ);
        videoDecoder.stcSendRequ(stcRequ); //just used for time to decode
        audioDecoder.stcSendRequ(stcRequ); //just used for time to decode
        demux.stcSendRequ(stcRequ); //just used for time to decode
        //stc --> stc offset (stc)
        //           --> pesDecoderVideo (stc)
        //           --> pesDecoderAudio (stc)
        //           --> demux (stc)
        stc.stcSend(stcStcChan);
        stcOffset.stcFromStc(stcStcChan);
        videoDecoder.stcGet(stcStcChan); //just used for time to decode
        audioDecoder.stcGet(stcStcChan); //just used for time to decode
        demux.stcGet(stcStcChan); //just used for time to decode

        // stcOffset <-- syncVideo (request)
        //           <-- syncAudio (request)
        //           <-- pesDecoderVideo (request)
        //           <-- pesDecoderAudio (request)
        //           <-- demuxPlayback (request)
        stcOffset.stcRequestFromModule(stcOffsetRequ);
        syncVideo.stcSendRequ(stcOffsetRequ);
        syncAudio.stcSendRequ(stcOffsetRequ);
        videoDecoder.stcSendRequOffset(stcOffsetRequ);
        audioDecoder.stcSendRequOffset(stcOffsetRequ);
        demux.stcSendRequOffset(stcOffsetRequ);
        // stcOffset --> syncVideo (stc)
        //           --> syncAudio (stc)
        //           --> pesDecoderVideo (stc)
        //           --> pesDecoderAudio (stc)
        //           --> demuxPlayback (stc)
        stcOffset.stcToModule(stcStcOffsetChan);
        syncVideo.stcGet(stcStcOffsetChan);
        syncAudio.stcGet(stcStcOffsetChan);
        videoDecoder.stcGetOffset(stcStcOffsetChan);
        audioDecoder.stcGetOffset(stcStcOffsetChan);
        demux.stcGetOffset(stcStcOffsetChan);

        //pesDecoderVideo --> syncVideo
        videoDecoder.pictureOut(pictureBuffer);
        syncVideo.frameIn(pictureBuffer);
        //syncVideo <-- outPutVideo
        syncVideo.frameRequest(outPutVideoRequ);
        outPutVideo.frameRequest(outPutVideoRequ);
        //syncVideo --> outPutVideo
        syncVideo.frameOut(outPutVideoGet);
        outPutVideo.frameIn(outPutVideoGet);


        //pesDecoderAudio --> syncAudio
        audioDecoder.audioOut(audioBuffer);
        syncAudio.frameIn(audioBuffer);
        //syncAudio <-- outPutAudio
        syncAudio.frameRequest(outPutAudioRequ);
        outPutAudio.frameRequest(outPutAudioRequ);
        //syncAudio --> outPutAudio
        syncAudio.frameOut(outPutAudioGet);
        outPutAudio.frameIn(outPutAudioGet);


    }
};



#endif /* MODULEBASIC_H_ */
