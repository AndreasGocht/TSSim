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

@file a few helper functions fot the test cases
'''

from stream_database import StreamDatabase
import multiprocessing
import os.path
import matplotlib
from lxml.html.builder import HEAD
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy
from Cheetah.Template import Template
import logging
import sys
import unittest




class Singleton(type):
    '''
    Singelton implementation for python
    '''
    _instances = {}
    def __call__(cls, *args, **kwargs):
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(*args, **kwargs)
        return cls._instances[cls]

class TestEnviroment(object):
    '''
    Singelton. This Singelton hold the basic configuration.
    Impotant values:
        self.simulator path to the Simulator
        self.mainResultDir dir to save simulation results (.csv, .png and .html files)
        self.maxThreads amount of processors maximal available for multible simulations
        self.db StreamDatabase object of streamDatabase.db. Used for all queries to the Steam database.
            If streamDatabase.db not exists it the class tries to find a streamDatabase.json, with the Informations to the Streams.
            If streamDatabase.json dosn't exists either, exits 
    '''
    __metaclass__ = Singleton

    def __init__(self):
        self.simulator = "../build/simulator"
        if not os.path.isfile(self.simulator):
            print "could not find simulator on \"" + self.simulator + "\". Exiting."
            exit(-1)  
        
        self.mainResultDir = "./results"
        self.maxThreads = multiprocessing.cpu_count()
        self.db = StreamDatabase("streamDatabase")
        if not self.db.check():
            self.db.new()
            self.db.loadJson()
            if not self.db.check():
                print "could not create db. Exiting"
                exit(-1)
                

def plot(pathCsv,data,xlabel = "", ylabel = "", title = ""):
    '''
    function to create and save a plot.
    
    param pathCsv filename witch the pictur will be saved (a .png will be appended)
    param data data[:,0] data to plot for the x Axis
               data[:,1] data to plot for the y Axis
    param xlable string for the x label
    param ylable string for the y label
    param title string for the title
    
    return if everything worked path to png, otherwise log an error and return None 
    '''
    path = pathCsv + ".png"
    try:
        plt.figure(1)                        
        try:
            #check if there is just one pair of data in there
            if len(data) == 2:            
                plt.plot(data[0],data[1],"x")
            else:
                plt.plot(data[:,0],data[:,1])
        except IndexError:
            logging.warn("IndexError: %s",sys.exc_info()[1])
            logging.warn("in csv: %s",pathCsv)
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        plt.title(title,fontsize=12)
        plt.savefig(path)
        plt.close()
        return path
    except OverflowError:
        logging.error("OverflowError when plotting file: "+ path)
        plt.close()
        return None

def parsePicname(name):
    '''
    function to parse a Pictname of the forme
    "simName.moduleName.propertyName"
    
    param name of the form "simName.moduleName.propertyName"
    
    return (simName,moduleName,propertyName)
    '''
    simName = name[:name.find(".")]
    name = name[name.find(".")+1:]
    moduleName = name[:name.find(".")]
    name = name[name.find(".")+1:]
    propertyName = name[:name.find(".")]
    return (simName,moduleName,propertyName) 

def buildOutputHtml(testDir, simDirs, htmlHead, htmlDescription, pageTitle = "A first test", modelDescription = "model description"):
    '''
    This function gernerate the plots and all the data for the tamplate.
    
    testDir -- directory of the test
    simDirs -- dictonary containing the result directory and mote propertys for description or the head
        simDirs[key] -- key that identify a simulation 
        simDirs[key]["simDir"] -- directory to the results
        simDirs[key]["..."] -- more infromation used by head or desctription
    htmlHead -- function to build the head of the result. The returned String will be used.
        I could use the data out of simDirs[key]["..."].    
    htmlDescription -- function to build the description of the result. The returned String will be used.
        I could use the data out of simDirs[key]["..."].
    title -- the title of the html page
    modelDescription -- a description of the model, like a small text, or a picture
    
    htmlHead and htmlDescription have to have tow arguments:
    htmlHead(simDirs,key), htmlDescription(simDirs,key)
        simDirs -- will be the mentions simDirs dictonary
        key -- will be a key that acess a special property in the dictonary
        
    '''
    results = []
    for key in simDirs:
        files = os.listdir(simDirs[key]["simDir"])
        piclines = {}
        for file in files:
            if (".csv" in file[-4:]):
                pathToCsv = simDirs[key]["simDir"] + "/" + file
                with open(pathToCsv) as f:
                    head = f.readline()
                    head = head.strip('\n')
                    f.seek(0)
                    try:
                        data = numpy.loadtxt(f, delimiter=",", skiprows=1)
                    except StopIteration as e:
                        logging.warn("StopIteration error: %s", sys.exc_info()[1])
                        logging.warn("in csv: %s", pathToCsv)
                        data = numpy.zeros((2, 2))
                    except ValueError as e:
                        logging.warn("parsing Error in file: %s",pathToCsv)
                        raise e
                title = os.path.basename(pathToCsv)
                title = title.replace(".csv","")
                title = title.replace(".","\n")
                if (len(head.split(",")) == 2):
                    pict = plot(pathToCsv, data, xlabel = head.split(",")[0], ylabel = head.split(",")[1],title=title)
                else:
                    pict = plot(pathToCsv, data, xlabel = head, ylabel = "???error in splitting Head",title=title)
                    logging.warn("error in splitting Head in %s",pathToCsv)
                picname = os.path.basename(pathToCsv)
                simName, moduleName, propertyName = parsePicname(picname)
                if moduleName not in piclines:
                    piclines[moduleName] = []
                if pict is not None:
                    piclines[moduleName].append({"pic":os.path.relpath(pict, testDir), "description":propertyName})
                else:
                    piclines[moduleName].append({"pic":"", "description":"picture for " + pathToCsv + " could not be generated."})
        with open(simDirs[key]["simDir"] + "/stdout.log") as f:
            log = f.read()
        for pics in piclines:
            piclines[pics].sort(key=lambda x:x['pic'])
        
        results.append({"id":key, "head":htmlHead(simDirs,key)\
                        , "description":htmlDescription(simDirs,key)\
                        , "piclines":piclines, "log":log})
    
    results.sort(key=lambda x:x['id'])
    with open("./template.html") as f:
        template_html = f.read()
    template = Template(template_html)
    template.title = pageTitle
    template.model = modelDescription
    template.pathCSS = os.path.relpath("./layout.css", testDir)
    template.results = results
    with open(testDir + "/result.html", "w") as f:
        f.write(str(template))
        
class SimulatorBaseTest(unittest.TestCase):
    '''
    Simulation base class.
    '''
    def setUp(self):
        unittest.TestCase.setUp(self)
    
    def tearDown(self):
        unittest.TestCase.tearDown(self)
    
    def checkSimulation(self,simStatus):
        '''
        checks if the simulation succeeded.
        
        simStatus list of dicts in the From:
            simStat["process"] = exit value of the Simulation process (assert to 0)
            simStat["summary"]["Info"] = amount of Infos that could be found in the log file (not asserted)
            simStat["summary"]["Warning"] = amount of Warnings that could be found in the log file (not asserted)
            simStat["summary"]["VideoStutter"] = amount of VideoStutterfound in the Log file. (not asserted)
            simStat["summary"]["Error"] = amount of Errors that could be found in the log file (assert to 0)
            simStat["summary"]["Fatal"] = amount of Fatal that could be found in the log file (assert to 0)
            
        '''
        reportFail = False 
        reportLogError = False
        reportLogFatal = False
        
        for simStat in simStatus:
            if simStat == {}:
                continue
            if simStat["process"] != 0:
                logging.error("Error in Simulation. Log could be found in: %s ,simulation finished with: %i",simStat["logfile"],simStat["process"])
                reportFail = True
            if not simStat["summary"]["Info"] == 0:
                logging.info("%i Infos found in Simulation. Log could be found in: %s.",simStat["summary"]["Info"],simStat["logfile"])            
            if not simStat["summary"]["Warning"] == 0:
                logging.warn("%i Warnings found in Simulation. Log could be found in: %s.",simStat["summary"]["Warning"],simStat["logfile"])
            if not simStat["summary"]["VideoStutter"] == 0:
                logging.warn("Video stutter found in Simulation. Log could be found in: %s.",simStat["logfile"])                                
            if not simStat["summary"]["Error"] == 0:
                logging.error("%i Errors found in Simulation. Log could be found in: %s.",simStat["summary"]["Error"],simStat["logfile"])
                reportLogError = True
            if not simStat["summary"]["Fatal"] == 0:
                logging.error("%i Fatal Errors found in Simulation. Log could be found in: %s.",simStat["summary"]["Fatal"],simStat["logfile"])
                reportLogFatal = True
                                
        self.assertFalse(reportFail, "one or more simulations failed. See logging.")
        self.assertFalse(reportLogError, "An error has occurred in a Simulation. See logging.")
        self.assertFalse(reportLogFatal, "An a Fatal error has occurred in a Simulation. See logging.")


def checkConstantAudio(file):
    '''
    check if delta pts of audio is constrant. logg error if not.
    '''
    if not file["constantdPts"]:
        logging.warn("Using Audio stream with no constant delta between PTS. You might get a not real Audio Pipeline behavior for %s",file["stream"])            
