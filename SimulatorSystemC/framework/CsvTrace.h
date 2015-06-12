/**
 * Copyright (c) 2012 Digisoft.tv Ltd.
 *
 * @file implementation of a CSV trace class for systemC
 *
 */

#ifndef CVSTRACE_H_
#define CVSTRACE_H_

#include "systemc.h"
#include <string>
#include <vector>
#include <stdio.h>
#include <fstream>
#include <iomanip>

using namespace std;

/** @brief this class is the base class for a trace.
 *
 * It creates a File with the given name, and creat the first line (description line)
 *
 */
class PropertyCsvTrace {
public:
    /** @brief creat a file with the given name, and writes the first line with xlable and ylable
     *
     * @param dir directory to create the file in
     * @param name name of the file. .csv is appended.
     * @param xlable the description of the first value in the CSV file
     * @param ylable the description of the second value in the CSV file
     */
    PropertyCsvTrace(const std::string dir, const string& name, string xlabel, string ylabel) {
        std::string filename(dir);
        filename.append("/");
        filename.append(name);
        filename.append(".csv");

        if (ylabel == "")
        {
            ylabel = name;
        }
        if (xlabel == "")
        {
            xlabel = "time";
        }


        m_file.open(filename,std::ofstream::out);

        if (!m_file) {
            std::string message;
            message += "Could not open  \"";
            message += name;
            message += "\". No logs will be saved";
            SC_REPORT_WARNING("/digisoft/simulator/PropertyCsvTrace", message.c_str());
        } else {
            m_file << std::setprecision(9) << std::fixed;
            m_file << xlabel << "," << ylabel << "\n";
        }
    };
    virtual ~PropertyCsvTrace() {
        m_file.close();
    };

    virtual void trace(double time) = 0;
    virtual bool changed() = 0;

protected:
    ofstream m_file;
};

/** @brief this class implements the sc_trace_file.
 *
 */
class CsvTrace: public sc_trace_file {
public:
    CsvTrace(const std::string name);
    virtual ~CsvTrace();

    void cycle(bool delta_cycle);

    void trace(const int& object, const std::string& allocator, int width = 8 * sizeof( int ))
    {
        trace(object, allocator, allocator);
    }
    void trace(const int&, const std::string&, string ylable = "");

    void trace(const double& object, const std::string& allocator)
    {
        trace(object, allocator, allocator);
    }
    void trace(const double&, const std::string&, string ylable = "");

    void trace(const bool& object, const std::string& allocator)
    {
        trace(object, allocator, allocator);
    }
    void trace(const bool&, const std::string&, string ylable = "");

    void trace(const unsigned long& object, const std::string& allocator, int width = 8 * sizeof( unsigned long ))
    {
        trace(object, allocator, allocator);
    }
    void trace(const unsigned long&, const std::string&, string ylable = "" );


    void trace(const long& object, const std::string& allocator,int width = 8 * sizeof( long ))
    {
        trace(object, allocator, allocator);
    }
    void trace(const long&, const std::string&, string ylable = "" );




    // trace transitions between delta cycles if flag is true.
    void delta_cycles(bool flag);

    //not implemented
    void trace(  const bool* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const float& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const float* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const double* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_logic& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_logic* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_int_base& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_int_base* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_uint_base& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_uint_base* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_signed& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_signed* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_unsigned& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_unsigned* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_bv_base& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_bv_base* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_lv_base& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_lv_base* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_fxval& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_fxval* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_fxval_fast& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_fxval_fast* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_fxnum& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_fxnum* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_fxnum_fast& , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_fxnum_fast* , const std::string& ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const unsigned char& , const std::string& , int width = 8 * sizeof( unsigned char ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const unsigned char* , const std::string& , int width = 8 * sizeof( unsigned char ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const unsigned short& , const std::string& , int width = 8 * sizeof( unsigned short ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const unsigned short* , const std::string& , int width = 8 * sizeof( unsigned short ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const unsigned int& , const std::string& , int width = 8 * sizeof( unsigned int ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const unsigned int* , const std::string& , int width = 8 * sizeof( unsigned int ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const unsigned long* , const std::string& , int width = 8 * sizeof( unsigned long ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const char& , const std::string& , int width = 8 * sizeof( char ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const char* , const std::string& , int width = 8 * sizeof( char ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const short& , const std::string& , int width = 8 * sizeof( short ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const short* , const std::string& , int width = 8 * sizeof( short ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const int* , const std::string& , int width = 8 * sizeof( int ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const long* , const std::string& , int width = 8 * sizeof( long ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::int64& , const std::string& , int width = 8 * sizeof( sc_dt::int64 ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::int64* , const std::string& , int width = 8 * sizeof( sc_dt::int64 ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::uint64& , const std::string& , int width = 8 * sizeof( sc_dt::uint64 ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::uint64* , const std::string& , int width = 8 * sizeof( sc_dt::uint64 ) ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_signal_in_if<char>& , const std::string& , int width ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_signal_in_if<short>& , const std::string& , int width ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_signal_in_if<int>& , const std::string& , int width ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_signal_in_if<long>& , const std::string& , int width ) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const sc_dt::sc_bit&, const std::string&) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void trace(  const unsigned int&, const std::string&, const char**) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void write_comment(const std::string&) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }
    void set_time_unit(double, sc_core::sc_time_unit) {
        SC_REPORT_WARNING("/digisoft/simulator/CsvTrace", "not implemented");
    }


private:
    std::vector<PropertyCsvTrace*> m_traces;
    const std::string m_dir;

    bool m_trace_delta_cycles = false;
};

#endif /* CVSTRACE_H_ */
