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

a few sample simulations. For Configruation values pleas look to the Documentation in the GitHub Wiki
'''
import unittest
import logging
import os
from helper_functions.process_handler import ProcessHandler  
import helper_functions.test_helper as th
import shutil
logging.basicConfig(format='%(asctime)s:%(levelname)s:%(name)s:%(filename)s:%(message)s', level=logging.INFO)


class Test(th.SimulatorBaseTest):
    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_pipeline_sintel(self):
        '''
        configure the basic pipeline
        '''
                
        testEnviroment = th.TestEnviroment()
        files = testEnviroment.db.configGetFile("sintel")
        testDir = testEnviroment.mainResultDir + "/test_pipeline_sintel"
        shutil.rmtree(testDir, ignore_errors = True)
        
        config = {}
        config["mainModel"] = "ModelBasic"
        config["ModelBasic.read"] = {}
        config["ModelBasic.read"]["trace"] = False
        config["ModelBasic.demuxInBuffer"] = {}
        config["ModelBasic.demuxInBuffer"]["size"] = 1
        config["ModelBasic.demuxInBuffer"]["trace"] = False
        config["ModelBasic.demux"] = {}
        config["ModelBasic.demux"]["trace"] = True
        config["ModelBasic.stc"] = {}
        config["ModelBasic.stc"]["pcrJumpBorder"] = 100000000 # 3.7s 
        config["ModelBasic.stc"]["trace"] = True
        config["ModelBasic.stcOffset"] = {}
        config["ModelBasic.stcOffset"]["offset"] = 8000000 #88.8 s * 90e3Hz
        config["ModelBasic.stcOffset"]["trace"] = False 
        config["ModelBasic.videoDecoderBuffer"] = {}
        config["ModelBasic.videoDecoderBuffer"]["trace"] = True
        config["ModelBasic.videoDecoderBuffer"]["size"] = 3 * 1024 * 1024
        config["ModelBasic.videoDecoder"] = {}
        config["ModelBasic.videoDecoder"]["trace"] = True
        config["ModelBasic.videoDecoder"]["decodingTime"] = 0.005 #5ms
        config["ModelBasic.pictureBuffer"] = {} 
        config["ModelBasic.pictureBuffer"]["trace"] = True
        config["ModelBasic.syncVideo"] = {}
        config["ModelBasic.syncVideo"]["trace"] = False
        config["ModelBasic.outPutVideo"] = {}
        config["ModelBasic.outPutVideo"]["trace"] = True
        config["ModelBasic.audioDecoderBuffer"] = {}
        config["ModelBasic.audioDecoderBuffer"]["size"] = 1 * 1024 * 1024
        config["ModelBasic.audioDecoderBuffer"]["trace"] = True
        config["ModelBasic.audioDecoder"] = {}
        config["ModelBasic.audioDecoder"]["trace"] = True
        config["ModelBasic.audioBuffer"] = {}
        config["ModelBasic.audioBuffer"]["trace"] = True
        config["ModelBasic.syncAudio"] = {}
        config["ModelBasic.syncAudio"]["trace"] = False
        config["ModelBasic.outPutAudio"] = {}
        config["ModelBasic.outPutAudio"]["trace"] = True


        processes = ProcessHandler(testEnviroment.maxThreads, testEnviroment.simulator)
        simDirs = {}
        simStatus = []
            
        for file in files:
            config["runTime"] = int(file["duration"])
            config["ModelBasic.read"]["filename"] = file["stream"]
            config["ModelBasic.read"]["bitRate"] = file["overallBitrate"]
            config["ModelBasic.demux"]["videoPid"] = file["videoPid"]
            config["ModelBasic.demux"]["audioPid"] = file["audioPid"]
            config["ModelBasic.demux"]["pcrPid"] = file["pcrPid"]
            config["ModelBasic.videoDecoder"]["videoTyp"] = file["videoBitStreamFormat"]
            config["ModelBasic.outPutVideo"]["framerate"] = float(file["frameRate"])
            config["ModelBasic.pictureBuffer"]["size"] = int(4000*1024*1024 / (file["width"]*file["height"]*1.5))
            config["ModelBasic.outPutAudio"]["framerate"] = 1/(float(file["mindPts"])/90e3)
            config["ModelBasic.audioBuffer"]["size"] = int(20*1024*1024/(float(file["mindPts"])/90e3 * 48e3 * 2))
            simDir = testDir + "/" + str(file["id"]) + "/v_" + str(file["videoPid"]) + "_a_" + str(file["audioPid"])
            key = str(file["id"]) + str(file["videoPid"]) + str(file["audioPid"])
            simDirs[key] = {"simDir": simDir,"file": file}
            simStatus.append(processes.spawn(simDir, config))
        simStatus.extend(processes.wait())
        
        def head(simDirs,key):
            return "simulation for file " + os.path.basename(simDirs[key]["file"]["stream"])
        
        def description(simDirs,key):
            return "The File for the Simulation is " + \
                        simDirs[key]["file"]["stream"] + "<br /> Video pid is: " + \
                        str(simDirs[key]["file"]["videoPid"]) + "<br /> Audio pid is: " + \
                        str(simDirs[key]["file"]["audioPid"]) + "<br /> Pcr pid is: " + \
                        str(simDirs[key]["file"]["pcrPid"]) + "<br /> name of the program: "+ \
                        str(simDirs[key]["file"]["programName"]) + "<br /> width x height " + \
                        str(simDirs[key]["file"]["width"]) + " x " + str(simDirs[key]["file"]["height"]) + "<br /> min audio length: " + \
                        str(simDirs[key]["file"]["mindPts"]) + "<br /> which is constant: " + \
                        str(simDirs[key]["file"]["constantdPts"])
                        
        
        th.buildOutputHtml(testDir, simDirs, head, description,"Simulation of a test pipeline","Simulation of a test pipeline")
        self.checkSimulation(simStatus)

    def test_pipeline_bigbuckbunny(self):
        '''
        configure the basic pipeline
        '''
                
        testEnviroment = th.TestEnviroment()
        files = testEnviroment.db.configGetFile("bbb")
        testDir = testEnviroment.mainResultDir + "/test_pipeline_bigbuckbunny"
        shutil.rmtree(testDir, ignore_errors = True)
        
        config = {}
        config["mainModel"] = "ModelBasic"
        config["ModelBasic.read"] = {}
        config["ModelBasic.read"]["trace"] = False
        config["ModelBasic.demuxInBuffer"] = {}
        config["ModelBasic.demuxInBuffer"]["size"] = 1
        config["ModelBasic.demuxInBuffer"]["trace"] = False
        config["ModelBasic.demux"] = {}
        config["ModelBasic.demux"]["trace"] = True
        config["ModelBasic.stc"] = {}
        config["ModelBasic.stc"]["pcrJumpBorder"] = 100000000
        config["ModelBasic.stc"]["trace"] = True
        config["ModelBasic.stcOffset"] = {}
        config["ModelBasic.stcOffset"]["offset"] = 0 
        config["ModelBasic.stcOffset"]["trace"] = False 
        config["ModelBasic.videoDecoderBuffer"] = {}
        config["ModelBasic.videoDecoderBuffer"]["trace"] = True
        config["ModelBasic.videoDecoderBuffer"]["size"] = 3 * 1024 * 1024
        config["ModelBasic.videoDecoder"] = {}
        config["ModelBasic.videoDecoder"]["decodingTime"] = 0.005 #5ms        
        config["ModelBasic.videoDecoder"]["trace"] = True
        config["ModelBasic.pictureBuffer"] = {} 
        config["ModelBasic.pictureBuffer"]["trace"] = True
        config["ModelBasic.syncVideo"] = {}
        config["ModelBasic.syncVideo"]["trace"] = False
        config["ModelBasic.outPutVideo"] = {}
        config["ModelBasic.outPutVideo"]["trace"] = True
        config["ModelBasic.audioDecoderBuffer"] = {}
        config["ModelBasic.audioDecoderBuffer"]["size"] = 1 * 1024 * 1024        
        config["ModelBasic.audioDecoderBuffer"]["trace"] = True
        config["ModelBasic.audioDecoder"] = {}
        config["ModelBasic.audioDecoder"]["trace"] = True
        config["ModelBasic.audioBuffer"] = {}
        config["ModelBasic.audioBuffer"]["trace"] = True
        config["ModelBasic.syncAudio"] = {}
        config["ModelBasic.syncAudio"]["trace"] = False
        config["ModelBasic.outPutAudio"] = {}
        config["ModelBasic.outPutAudio"]["trace"] = True


        processes = ProcessHandler(testEnviroment.maxThreads, testEnviroment.simulator)
        simDirs = {}
        simStatus = []
            
        for file in files:
            config["runTime"] = int(file["duration"])
            config["ModelBasic.read"]["filename"] = file["stream"]
            config["ModelBasic.read"]["bitRate"] = file["overallBitrate"]
            config["ModelBasic.demux"]["videoPid"] = file["videoPid"]
            config["ModelBasic.demux"]["audioPid"] = file["audioPid"]
            config["ModelBasic.demux"]["pcrPid"] = file["pcrPid"]
            config["ModelBasic.videoDecoder"]["videoTyp"] = file["videoBitStreamFormat"]
            config["ModelBasic.outPutVideo"]["framerate"] = float(file["frameRate"])
            config["ModelBasic.pictureBuffer"]["size"] = int(40*1024*1024 / (file["width"]*file["height"]*1.5))
            config["ModelBasic.outPutAudio"]["framerate"] = 1/(float(file["mindPts"])/90e3)
            config["ModelBasic.audioBuffer"]["size"] = int(20*1024*1024/(float(file["mindPts"])/90e3 * 48e3 * 2))
            simDir = testDir + "/" + str(file["id"]) + "/v_" + str(file["videoPid"]) + "_a_" + str(file["audioPid"])
            key = str(file["id"]) + str(file["videoPid"]) + str(file["audioPid"])
            simDirs[key] = {"simDir": simDir,"file": file}
            simStatus.append(processes.spawn(simDir, config))
        simStatus.extend(processes.wait())
        
        def head(simDirs,key):
            return "simulation for file " + os.path.basename(simDirs[key]["file"]["stream"])
        
        def description(simDirs,key):
            return "The File for the Simulation is " + \
                        simDirs[key]["file"]["stream"] + "<br /> Video pid is: " + \
                        str(simDirs[key]["file"]["videoPid"]) + "<br /> Audio pid is: " + \
                        str(simDirs[key]["file"]["audioPid"]) + "<br /> Pcr pid is: " + \
                        str(simDirs[key]["file"]["pcrPid"]) + "<br /> name of the program: "+ \
                        str(simDirs[key]["file"]["programName"]) + "<br /> width x height " + \
                        str(simDirs[key]["file"]["width"]) + " x " + str(simDirs[key]["file"]["height"]) + "<br /> min audio length: " + \
                        str(simDirs[key]["file"]["mindPts"]) + "<br /> which is constant: " + \
                        str(simDirs[key]["file"]["constantdPts"])
                        
        
        th.buildOutputHtml(testDir, simDirs, head, description,"Simulation of a test pipeline","Simulation of a test pipeline")
        self.checkSimulation(simStatus)

        
if __name__ == "__main__":
    #import sys;sys.argv = ['', 'Test.testName']
    unittest.main()