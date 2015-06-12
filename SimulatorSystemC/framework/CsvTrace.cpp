/**
 * Copyright (c) 2012 Digisoft.tv Ltd.
 *
 * @file implementation of a CSV trace class for systemC
 *
 */
#include "CsvTrace.h"


/** @brief this class implements the PropertyCsvTrace class for different typs of values
 *
 */
template<class T>
class PropertyCsvTraceT: public PropertyCsvTrace {
public:

    /** @brief reimpelemtnation of PropertyCsvTrace::PropertyCsvTrace(dir, name, xlabel, ylabel)
     *
     * @param dir directory to create the file in
     * @param name name of the file. .csv is appended.
     * @param object ocject to trace.
     * @param xlable the description of the first value in the CSV file
     * @param ylable the description of the second value in the CSV file
     */
    PropertyCsvTraceT(const std::string dir, const string& name, const T& object, string xlabel, string ylabel):
        PropertyCsvTrace(dir, name, xlabel, ylabel),
        m_object(object),
        m_oldObject(object) {
    }

    /**
     * @brief writes the Trace to a file.
     *
     * This function is called by CsvTrace::cycle to trace a Variable. It is called every time a Value had changed.
     *
     * @param time time on witch the tracing happens.
     *
     */
    void trace(double time) {
        if (m_file) {
            m_file << time << "," << m_object << "\n";
        }

        m_oldObject = m_object;
    }

    /**
     * @brief determine if object had changed.
     *
     * return true if the object has changed, false otherwise.
     *
     */
    bool changed() {
        return m_oldObject != m_object;
    }

private:
    const T &m_object;
    T m_oldObject;
};

/**
 * @brief create and register the tracer against systemC.
 *
 * @param dir directory to save csv files.
 *
 */
CsvTrace::CsvTrace(const std::string dir):
    m_dir(dir)
{
    sc_get_curr_simcontext()->add_trace_file( this );
}

/**
 * @brief destroy all trace objects.
 *
 */
CsvTrace::~CsvTrace()
{
    for (uint i = 0; i < m_traces.size(); i++) {
        delete m_traces[i];
    }
}

/**
 * @brief triger tracing of all associated traces.
 *
 * This function is calles by systemC after each evaluation cycle.
 *
 * if delta_cycles are enabled it traces even inside on time step, otherwise just at the end of a timestep.
 *
 * It checks if a value hase chagned, and trace it if so.
 *
 * @param delta_cycle given by systemC to determine if we are in a delta cycle or not.
 *
 */
void CsvTrace::cycle(bool delta_cycle)
{
    if (delta_cycle && !this->m_trace_delta_cycles) {
        return;
    }

    for (uint i = 0; i < m_traces.size(); i++) {
        if (m_traces[i]->changed()) {
            m_traces[i]->trace(sc_time_stamp().to_seconds());
        }
    }
}

/**
 * @brief function to add an integer variable to tracing
 *
 * @param object reference to the variable that should be traced.
 * @param object name of the variable that should be traced.
 * @param object description of the variable that sould be traces (for example Units).
 *
 */
void CsvTrace::trace(const int& object, const std::string& allocator, string ylabel)
{
    PropertyCsvTrace* newtrace = new PropertyCsvTraceT<int>(m_dir, allocator, object, "time in seconds", ylabel);
    m_traces.push_back(newtrace);
}

/**
 * @brief function to add an double variable to tracing
 *
 * @param object reference to the variable that should be traced.
 * @param object name of the variable that should be traced.
 * @param object description of the variable that sould be traces (for example Units).
 *
 */
void CsvTrace::trace(const double& object, const std::string& allocator,string ylabel)
{
    PropertyCsvTrace* newtrace = new PropertyCsvTraceT<double>(m_dir, allocator, object, "time in seconds", ylabel);
    m_traces.push_back(newtrace);
}

/**
 * @brief function to add an bool variable to tracing
 *
 * @param object reference to the variable that should be traced.
 * @param object name of the variable that should be traced.
 * @param object description of the variable that sould be traces (for example Units).
 *
 */
void CsvTrace::trace(const bool& object, const std::string& allocator, string ylabel)
{
    PropertyCsvTrace* newtrace = new PropertyCsvTraceT<bool>(m_dir, allocator, object, "time in seconds", ylabel);
    m_traces.push_back(newtrace);
}

/**
 * @brief function to add an unsinged long variable to tracing
 *
 * @param object reference to the variable that should be traced.
 * @param object name of the variable that should be traced.
 * @param object description of the variable that sould be traces (for example Units).
 *
 */
void CsvTrace::trace(const unsigned long& object, const std::string& allocator, string ylabel)
{
    PropertyCsvTrace* newtrace = new PropertyCsvTraceT<unsigned long>(m_dir, allocator, object, "time in seconds", ylabel);
    m_traces.push_back(newtrace);
}

/**
 * @brief function to add an long variable to tracing
 *
 * @param object reference to the variable that should be traced.
 * @param object name of the variable that should be traced.
 * @param object description of the variable that sould be traces (for example Units).
 *
 */
void CsvTrace::trace(const long& object, const std::string& allocator, string ylabel)
{
    PropertyCsvTrace* newtrace = new PropertyCsvTraceT<long>(m_dir, allocator, object, "time in seconds", ylabel);
    m_traces.push_back(newtrace);
}


/**
 * @brief function to activate/deactivate delta cycle tracing
 *
 * @param flag if true activate delta cycles (default false).
 */
void CsvTrace::delta_cycles(bool flag)
{
    this->m_trace_delta_cycles = flag;
}
