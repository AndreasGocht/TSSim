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
 * @file Main file, for setup of the Simulation.
 *
 */
#include <framework/MainModel.h>
#include "systemc.h"
#include "time.h"
#include "framework/Configuration.h"
#include "framework/CsvTrace.h"
#include <string>

/**
 * @brief main function called by SystemC (the main() function is in systemC)
 *
 * @param argc given by systemC (normal main(argc, argv*[]))
 * @param argv given by systemC (normal main(argc, argv*[]))
 *
 * (optional) @returns Description of return value.
 */
int sc_main (int argc, char* argv[])
{
    if (argc != 2) {
        cout << "Wrong usage!" << "\n";
        cout << "run with: " << "\n";
        cout << "\t  simulator <in/out dir>" << "\n";
        exit(-1);
    }

    sc_report_handler::set_actions (SC_ID_MORE_THAN_ONE_SIGNAL_DRIVER_,
            SC_DO_NOTHING);

    std::string dir = argv[1];
    Configuration& config = Configuration::getInstance();

    if (!config.loadConfigFromDir(dir)) {
        exit(-1);
    }

    const std::shared_ptr<sc_module> mainModel = MainModel::getMainModel();

    if (!config.HasMember("runTime") || !config["runTime"].IsInt()) {
        std::string message;
        message += "No \"runTime:\" found, or not Int.";
        SC_REPORT_FATAL("/digisoft/simulator/BufferFill", message.c_str());
    }

    int runTime = config["runTime"].GetInt();
    clock_t t1;
    clock_t t2;
    t1 = clock();
    sc_start(runTime, SC_SEC);
    t2 = clock();
    std::string message;
    message += "used Time for the simulation: ";
    message += std::to_string(((float)t2 - t1) / CLOCKS_PER_SEC);
    message += " simulated Time: ";
    message += std::to_string(sc_time_stamp().to_seconds());
    SC_REPORT_INFO("/digisoft/simulator/main", message.c_str());
    return 0;
}
