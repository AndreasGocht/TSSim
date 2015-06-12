'''
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
 
@fiel this file hold a class to handel simulation prcosesses
'''
import os
import json
import subprocess
import time


class ProcessHandler(object):
    '''
    This Class is for creating config.json and stdOut files as well as for spawning the processes and waiting for them.
    call spawn() to start a new simulation
    call wait() to wait till all simulations are finished.
    
    This class make some basic assertions, to check if the program had finished successfully.
    '''


    def __init__(self, maxThreads, simulator):
        '''
        Constructor
        maxThreads [in] amount of processes that should been used
        simulator [in] path to the simulator to start
        '''
        self.remainingThreads = maxThreads
        self.processes = []
        self.simulator = simulator
        
    def spawn(self, simDir, config):
        '''
        This function create the directory simDir, save the configuration and create the stdout file.
        If there are free process slots it spans the simulation. If not it waits till there is a free slot.
        
        simDir [in] dir to save configfile, stdOut file, and to pass to the simulation for output
        config [in] directory with the configuration for the simulation
        
        return result["process"] exit stat of the process
               result["logfile"] stdOut log file of the process
               result["summary"] return a Dictonary that give a summary of the logfile:
               result["summary"]["Info"]
               result["summary"]["Warning"]
               result["summary"]["Error"]
               result["summary"]["Fatal"]
                       they all hold a number with occurence of the equivalent logging message
               OR
               {} if process is just started
        '''
        result = {}
        
        try:
            os.makedirs(simDir)
        except OSError:
            pass
                    
        with open(simDir + "/config.json", "w") as file:
            json.dump(config, file, indent=4, separators=(',', ': '))
            
        #free processes?
        if (self.remainingThreads > 0):
            newProcess = {}
            newProcess['stdOutfile'] = open(simDir + "/stdout.log", "w")
            newProcess['process'] = subprocess.Popen([self.simulator, simDir], stdout=newProcess['stdOutfile'], stderr=newProcess['stdOutfile'])
            self.processes.append(newProcess)
            self.remainingThreads = self.remainingThreads - 1
        else:
            #no? wait for free one
            found = False
            
            while(not found):
                time.sleep(0.1)
                for key, process in enumerate(self.processes):
                    if process['process'].poll() is not None:
                        result['process'] = process['process'].poll()
                        result['logfile'] = process['stdOutfile'].name
                        result['summary'] = self.parseLogFile(process['stdOutfile'].name)
                        process['stdOutfile'].close()
                        newProcess = {}
                        newProcess['stdOutfile'] = open(simDir+"/stdout.log","w")
                        newProcess['process'] = subprocess.Popen([self.simulator, simDir], stdout=newProcess['stdOutfile'], stderr=newProcess['stdOutfile'])
                        self.processes[key] = newProcess
                        found = True
        return result

    def wait(self):
        '''
        this function wait for all Simulations to finish.
        
        return a list of dict:
            result["process"] exit stat of the process
            result["logfile"] stdOut log file of the process
            result["summary"] return a Dictonary that give a summary of the logfile:
            result["summary"]["Info"]
            result["summary"]["Warning"]
            result["summary"]["Error"]
            result["summary"]["Fatal"]
                they all hold a number with occurence of the equivalent logging message
            
        '''
        result = []
        stillActive = 1
        while(stillActive > 0):
            stillActive = 0
            for process in self.processes:
                if process['process'].poll() is None:
                    stillActive = stillActive + 1
            time.sleep(0.1)
        
        for process in self.processes:
            result.append({'process':process['process'].poll() , 'logfile' : process['stdOutfile'].name, 'summary' : self.parseLogFile(process['stdOutfile'].name)})
        
        return result
    
    def parseLogFile(self, logFile):
        '''
        This function parse the logFile. It will look for Keywords and count their occurrence.
        
        return diconary in the form:
            result["Info"] = number of occurrences
            result["Warning"] = number of occurrences
            result["Error"] = number of occurrences
            result["Fatal"] = number of occurrences
            result["VideoStutter"] = number of occurrences
        '''
        messages = {"Info":"Info: ", "Warning":"Warning: ", "Error":"Error: ", "Fatal":"Fatal: ", "VideoStutter":"outPutVideo: stutter occurred"}
        result = {}
        with open(logFile) as f:
            log = f.read()
        for message in messages:
            result[message] = log.count(messages[message])
        return result
