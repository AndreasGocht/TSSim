/**
 * Copyright (c) 2012 Digisoft.tv Ltd.
 *
 * @file this file cares about selecting the moddel.
 *
 */

#ifndef MAINMODULE_H_
#define MAINMODULE_H_

#include <modules/models/ModelBasic.h>
#include <string>

#include "systemc.h"
#include "Configuration.h"

#define MAIN_MODEL "mainModel"

namespace MainModel {
    /** @brief this function chooses the moddel we war using for the simulation
     *
     * It checks the configuration file and choose the moddel to simulate according to the Simulaion
     *
     * @return an sc_module that is holding the Model
     *
     */
    const static std::shared_ptr<sc_module> getMainModel() {
        Configuration& config = Configuration::getInstance();

        if (!config.HasMember(MAIN_MODEL) || !config[MAIN_MODEL].IsString()) {
            std::string message;
            message += "Malformed configuration. \"";
            message += MAIN_MODEL;
            message += "\" is missing or no String";
            SC_REPORT_FATAL("/digisoft/simulator/MainModule", message.c_str());
        }

        std::string id = config[MAIN_MODEL].GetString();

        if (id == "ModelBasic") {
            return std::make_shared<ModelBasic>("ModelBasic");
        } else {
            std::string message;
            message += "Malformed configuration. \"";
            message += id;
            message += "\" not found";
            SC_REPORT_FATAL("/digisoft/simulator/MainModule", message.c_str());
            exit(-1);
        }
    }
};

#endif /* MAINMODULE_H_ */
