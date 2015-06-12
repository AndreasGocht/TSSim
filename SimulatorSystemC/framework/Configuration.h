/**
 * Copyright (c) 2012 Digisoft.tv Ltd.
 *
 * @file Holds the implementation of the Configuration sigelton.
 *
 */
#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "rapidjson/document.h"
#include "systemc.h"
#include <string>
#include <fstream>
#define MODULE_ID_STR "/digisoft/simulator/framework/Configuration"

class Configuration: public rapidjson::Document {
public:
    static Configuration& getInstance() {
        static Configuration instance;
        return instance;
    }

    /**
     * @brief this function loads a .json file.
     *
     * @param dir directory that contains a "/config.json" file, witch holds the configuratuion for the simulation
     *
     * @returns true if the file could been loaded, false otherwise..
     */
    bool loadConfigFromDir(std::string dir) {
        this->m_dir = dir;
        std::string configFile = dir;
        configFile.append("/config.json");
        ifstream file;
        char configFileCache[parseJsonBufferSize];
        file.open(configFile, std::ifstream::in);

        if (!file) {
            string message = "config file \"";
            message += configFile;
            message += "\" could not been loaded";
            SC_REPORT_FATAL(MODULE_ID_STR,message.c_str());
            return false;
        }

        file.read(configFileCache, parseJsonBufferSize);
        this->Parse(configFileCache);
        return true;
    }

    std::string dir() {
        return this->m_dir;
    }

private:
    std::string m_dir;
    static const int parseJsonBufferSize = 64000;
    Configuration() {};
    Configuration(Configuration const&);
    void operator=(Configuration const&);
};
#undef MODULE_ID_STR


#endif /* CONFIGURATION_H_ */
