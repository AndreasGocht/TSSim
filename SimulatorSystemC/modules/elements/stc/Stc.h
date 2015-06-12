/*
 * 
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

#ifndef STC_H_
#define STC_H_


#include "systemc.h"
#include <cmath>
#include <vector>
#include <stdint.h>
#include "framework/CsvTrace.h"
#include "framework/Configuration.h"

#define MODULE_ID_STR "/digisoft/simulator/modules/elements/stc/stc"

SC_MODULE(Stc)
{
    sc_in<int64_t> pcrIn;
    sc_in<bool> stcRequest;
    sc_out<int64_t> stcSend;
    sc_inout<bool> startStc;

    // variables for error logging
    double middelError;
    int middelErrorN;
    int64_t plainError;
    int64_t incommingPcr;
    int64_t pcrJumpBorder;
    bool stcRunning = false;


private:
    std::shared_ptr<CsvTrace> m_csvTrace;

    int stcFrequency = 27e6;
    int64_t offset = 0;
    int64_t pcrBase(double t) {
        return int64_t((stcFrequency * t) / 300) % int64_t(pow(2, 33));
    }
    int64_t pcrExt(double t) {
        return int64_t((stcFrequency * t) / 1) % 300 ;
    }
    int64_t pcr(double t) {
        return pcrBase(t) * 300 + pcrExt(t);
    }
    int64_t stc(double t) {
        return pcr(t) + offset;
    }

public:

    /** @brief loads the configuration of the buffer out of the to the program given .json file.
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

        if (!s.HasMember("pcrJumpBorder") || !s["pcrJumpBorder"].IsInt64()) {
            std::string message;
            message += "Malformed configuration of \"";
            message += this->name();
            message += "\". \"pcrJumpBorder\" is missing or no Int64";
            SC_REPORT_FATAL(MODULE_ID_STR, message.c_str());
        }
        this->pcrJumpBorder = s["pcrJumpBorder"].GetInt64();

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
                m_csvTrace->trace(middelError,std::string(this->name()).append(".middelError"),"average error in PCR counts");
                m_csvTrace->trace(incommingPcr,std::string(this->name()).append(".incommingPcr"),"values of incoming PCR");
                m_csvTrace->trace(plainError,std::string(this->name()).append(".plainError"),"error in PCR counts");
                m_csvTrace->trace(stcRunning,std::string(this->name()).append(".stcRunning"),"bool if stc is started/running");
            }
        }

    }


    /** @brief this function update the inernatl stc, with the current pcr values
     *
     * This function receives pcr values. If there is a pcr discontinuity it will print a warning,
     * and reset the STC to the new pcr. Otherwise it will just count ahead.
     *
     * If the frequency of the sender is not exact 27MHz this might be caused as well, because a PCR
     * drift is not considered
     */
    virtual void stcUpdateProc() {
        int64_t newPcr;
        int64_t lastPcr = 0;

        while (true) {
            wait(pcrIn.default_event());
            newPcr = pcrIn.read();
            incommingPcr = newPcr;

            // lastPcr > newPcr means pcr warp around
            if ((offset == 0) || ( llabs(lastPcr - newPcr) > this->pcrJumpBorder )) {
                if(offset != 0)
                {
                    SC_REPORT_WARNING(MODULE_ID_STR,"pcr jump or warp around");
                }
                offset = newPcr - pcr(sc_time_stamp().to_seconds());
                std::string message;
                message += "set new offset: ";
                message += std::to_string(offset);
                SC_REPORT_INFO(MODULE_ID_STR, message.c_str());
            } else {
                int64_t error = 0;
                error = newPcr - stc(sc_time_stamp().to_seconds());
                estimatError(error);
            }
            lastPcr = newPcr;
        }
    }


    /** @brief this function compute a middle stc error.
     *
     * @param error the total stc error
     *
     */
    void estimatError(int64_t error)
    {
        plainError = error;
        middelError = sqrt((pow(middelError,2) * middelErrorN + pow(error,2))/(middelErrorN +1));
        middelErrorN++;
    }


    /** @brief: this function return the actuel stc value
     *
     * If the stc is enabled by wrting true into @stcStarted it will retrun the stc in
     * 90khz steps.
     *
     */
    virtual void stcRequestProc() {
        startStc.write(false);
        while (true) {

            /*
             * check if we are already up to start the stc. If the STC is enabled, wait for requests.
             * This is to prevent the proces from infinite running.
             * */
            if(!stcRunning)
            {
                SC_REPORT_INFO(MODULE_ID_STR,"stc disabled");
                wait(startStc.default_event());
                SC_REPORT_INFO(MODULE_ID_STR,"stc enabled");
            }
            else
            {
                wait(stcRequest.default_event());
            }

            if (stcRequest.read()) {
                stcSend.write(stc(sc_time_stamp().to_seconds())/300);
            }
        }
    }

    /** @brief: this function is called whenever startStc is changes. It changes the stcRunning status
     *
     */
    void startStcProc()
    {
        stcRunning = startStc.read();
    }

    SC_CTOR(Stc) {
        this->loadConfig();
        SC_THREAD(stcUpdateProc);
        SC_THREAD(stcRequestProc);

        /* Methond for changing the stc state */
        SC_METHOD(startStcProc);
        sensitive << startStc;
    }
};

#undef MODULE_ID_STR

#endif //STC_H_





