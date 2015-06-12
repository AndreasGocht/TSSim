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

This module holds a lot of functions to help the StreamDatabase to index ts Files using bitStream
'''


from bitstream.bitstream import bs, bsNULL, bsString
import os
import logging

#MAXPCR = 2**33*300 + 2**9
MAXPCR = 2576980378112


def findSync(tsStream):
    '''find start of tsPacket
    '''
    foundSyncByte = False
    while not foundSyncByte:
        data = tsStream.read(1) 
        if (ord(data) == 0x47):
            foundSyncByte = True
            logging.debug("found Sync Byte at %i", tsStream.tell()-1)
            tsStream.seek(-1, 1)

def getTsPacket(tsStream):
    '''function to get the tsPacket.
    
    This function ensures that the packet is a valid tsPacket. It return the readed data
    '''
    data = tsStream.read(188)
    # when we are at the end of the file, we just return the data. Otherwise we would strugle
    # with the back jumps
    if len(data) < 188:
        return data
    if not bs.ts_validate(data):
        tsStream.seek(-188, 1)
        findSync(tsStream)
        data = tsStream.read(188)
    return data

def calculateBitrateAndDuration(filename):
    '''calculate the overall Bitrate and the duration of the Stream.
    This function looks for the first and the last PCR. The bitrate is calculated using the filesize.
     
    filename -- path to the Stream
    return -- dictonary holding 
        result["bitrate"] the bitrate
        result["duration"] the duration measured in ms. 
     
    
    '''
    tsStream = open(filename, "rb")

    result = {}
    firstPcr = None
    firstPcrPid = None
    firstPcrPidPos = None
    lastPcr = None 
    lastPcrPos = None
    
    while True:
        tsPacket = getTsPacket(tsStream)
        if len(tsPacket) < 188:
            break
        pid = bs.ts_get_pid(tsPacket)
        if bs.ts_has_adaptation(tsPacket) and bs.ts_get_adaptation(tsPacket) != 0 and bs.tsaf_has_pcr(tsPacket):
            firstPcr = bs.tsaf_get_pcr(tsPacket) * 300 + bs.tsaf_get_pcrext(tsPacket)
            firstPcrPid = pid
            firstPcrPidPos = tsStream.tell()
            break
    
    
    #go to the end of the filename, and find the same TS packet again.
    #look for the PCR 
        
    size = os.path.getsize(filename)
    tsStream.seek(size - 1000000)
    
    #find tspackage beginning
    findSync(tsStream)
            
    while True:
        tsPacket = getTsPacket(tsStream)
        if len(tsPacket) < 188:
            break        
        pid = bs.ts_get_pid(tsPacket)
        if pid == firstPcrPid:
            if bs.ts_has_adaptation(tsPacket) and bs.ts_get_adaptation(tsPacket) != 0 and bs.tsaf_has_pcr(tsPacket):
                lastPcr = bs.tsaf_get_pcr(tsPacket) * 300 + bs.tsaf_get_pcrext(tsPacket)
                lastPcrPos = tsStream.tell()
    
    tsStream.close()
    
    measuredSize = lastPcrPos - firstPcrPidPos
    
    if lastPcr > firstPcr:
        diff = lastPcr - firstPcr
    else:
        # pcr overflow
        diff = (MAXPCR - firstPcr) + lastPcr
             
    result["bitrate"] = (measuredSize * 8) / (diff / 27000000)
    result["duration"] = (diff / 27000000.0) * 1000
    
    
    return result

def calculateMaxBitrate(filename, overAllbitRate, dt):
    '''this function calculate the maximal bitrate for each pid in the stream
    
    The function read 188 byte and add one timestep depending on the overAllbitRate to a internal time counter.
    When dt time has passed, the bitrate is calculated. If the already saved bitrate is smaller than the new one the new one is saved.
    
    The function assume a constant Bitrate.
    
    filename -- path to the stream
    overAllbitRate -- bitrate of the stream. Assumed to be constand
    dt -- time frame for calculating bitrate. Bitrate is calculated count/dt
    return -- dictonary holding the pid , together with bitrate and total amount of counted pacakges:
        result[pid]["maxBitRate"]
        result[pid]["countTotal"]
    '''
    result = {}
    time = 0
               
    tsStream = open(filename, "rb")
    findSync(tsStream)

    while tsStream:
        tsPacket = getTsPacket(tsStream)
        if len(tsPacket) < 188:
            break
        
        pid = bs.ts_get_pid(tsPacket)
        
        if pid not in result:
            result[pid] = {"countTotal": 0, "count": 0, "maxBitRate": None}
        
        result[pid]["count"] = result[pid]["count"] + 1
        result[pid]["countTotal"] = result[pid]["countTotal"] + 1
        time = time + (188 * 8 / overAllbitRate)

        if(time > dt):
            for pid in result:
                bitRate = (result[pid]["count"] * 188 * 8) / time
                if bitRate > result[pid]["maxBitRate"]:
                    result[pid]["maxBitRate"] = bitRate
                result[pid]["count"] = 0
            time = 0
                    
    tsStream.close()
    return result   


def findAndParsePAT(filename):
    ''' find the pat
    This function search the TS steam and find the PAT.
    
    filename -- name of file to search for pat
    
    return dictonary with menuId and pid:
        return[pid] = menuId
    '''
    logging.debug("open stream")
    tsStream = open(filename, "rb")
    logging.debug("search sync")
    findSync(tsStream)

    logging.debug("search for pat")
    found = False
    unitstartFound = False
    tsPacket = []
    result = {}  
    while not found:
        tsPacket = getTsPacket(tsStream)
        if len(tsPacket) < 188:
            logging.error("No pat found")
            break
        
        if (bs.ts_get_pid(tsPacket) == 0):
            section = bs.ts_next_section(tsPacket)
            if bs.pat_validate(section) and (unitstartFound or bs.ts_get_unitstart(tsPacket)):
                
                unitstartFound = True
                
                logging.debug("pat found at byte: %i", tsStream.tell())
                parsePAT(section, result)
                if bs.psi_get_lastsection(section) == bs.psi_get_section(section):
                    found = True
                else:
                    logging.debug("pat not finished")
    tsStream.close()
        
    return result

def parsePAT(section, result):
    ''' helperfunction for findAndParsePAT
    
    This function parse the PAT.
    
    section -- ts section containung the pat 
    result[in|Out] -- dictonary to store the result.
    
    return -- None
    '''
    logging.debug("parse pat")
    finished = False
    i = 0
    while (not finished) and i < 256:
        if not (bs.pat_get_program(section, i) == bsNULL): 
            result[bs.patn_get_pid(bs.pat_get_program(section, i))] = bs.patn_get_program(bs.pat_get_program(section, i))
            i = i + 1
        else:
            finished = True
            
        
def findAndParsePMTs(filename, menus):
    '''search in the file for the given PMT's.
    
    This function search and parse the file for the given pmt Pids.
    
    filename -- name of the file
    menus -- list or dict with pids to look for. If it is a dict the keys should be the pids
    
    return -- dictonary containing the pcrPid, and ohter pids with type:
        return[pid]["pcrPid"] = pcrPid
        return[pid]["pids"][esPid] = streamtype
    '''
    tsStream = open(filename, "rb")
    findSync(tsStream)

    remainingPids = list(menus)
    result = {}
    unitstartFound = {}
    
    for pid in remainingPids:
        unitstartFound[pid] = False
    
    
    while len(remainingPids) > 0:
        tsPacket = getTsPacket(tsStream)
        if len(tsPacket) < 188:
            logging.error("some PMTs not found")
            break
        
        pid = bs.ts_get_pid(tsPacket) 
        if (pid in remainingPids):
            
            section = bs.ts_next_section(tsPacket)
            if bs.pmt_validate(section) and (unitstartFound[pid] or bs.ts_get_unitstart(tsPacket)):
                
                unitstartFound[pid] = True
                
                logging.debug("pmt for pid %i found", pid)
                parsePMT(section, result, pid)
                
                if bs.psi_get_lastsection(section) == bs.psi_get_section(section):
                    remainingPids.remove(pid)
                else:
                    logging.debug("pat to pid %i not finished", pid)
                    
    tsStream.close()
    return result

def parsePMT(section, result, pid):
    '''helperfunction for findAndParsePMTs
    
    pars the given PMT
    
    section -- tsPacket section containing the PMT
    result[in|out] -- dictonary with the results:
        result[pid]["pcrPid"] = pcrPid
        result[pid]["pids"][esPid] = streamtype
        
    return -- None
    '''
    pcrPid = bs.pmt_get_pcrpid(section)
    pids = {}
    finished = False
    i = 0 
    while not finished:
        es = bs.pmt_get_es(section, i)
        if not (es == bsNULL):
            pids[bs.pmtn_get_pid(es)] = bsString(bs.pmt_get_streamtype_txt(bs.pmtn_get_streamtype(es)))
            i = i + 1
        else:
            finished = True
    result[pid]= {"pcrPid": pcrPid, "pids": pids}

def parseDeltaPtsAudio(filename, pids):
    '''
    this function looks if the difference between 2 pts stamps is constant.
    
    It is mostly interesting for Audio, as this property allows us to assume or not a constant "audio framerate"
    
    filename -- path to the file to search
    pids -- list of pids to calculate
    
    return -- a tuple of dicts in the form:
       return (mindpts,constantStream)
           mindpts[pid] -- Minimal delta between 2 pts.
           constantStream[pid] -- the stream has a constant delta between each pts, so the minimal one is the onliest one.
    '''
    
    pesPacket = {}
    pts = {}
    lastpts = {}
    dpts = {}
    lastdpts = {}
    constantStream = {}
    mindpts = {}
    
    for pid in pids:
        pesPacket[pid] = ""
        pts[pid] = 0
        lastpts[pid] = 0
        dpts[pid] = 0
        lastdpts[pid] = 0
        constantStream[pid] = True
        mindpts[pid] = 0
    
    logging.debug("open stream")
    tsStream = open(filename, "rb")
    logging.debug("search sync")
    findSync(tsStream)
    while tsStream:
        tsPacket = getTsPacket(tsStream)
        if len(tsPacket) < 188:
            break
        
        pid = bs.ts_get_pid(tsPacket)
        
        if pid not in pids:
            continue
        
        if bs.ts_get_unitstart(tsPacket):
            if bs.pes_validate(pesPacket[pid]):
                if bs.pes_validate_header(pesPacket[pid]) and bs.pes_has_pts(pesPacket[pid]) and bs.pes_validate_pts(pesPacket[pid]):
                    lastpts[pid] = pts[pid]
                    pts[pid] = bs.pes_get_pts(pesPacket[pid])
                    lastdpts[pid] = dpts[pid]
                    
                    if (pts[pid] > 0) and (lastpts[pid]>0):
                        dpts[pid] = pts[pid] - lastpts[pid]
                    
                    if (lastdpts[pid] != dpts[pid]) and (lastdpts[pid] > 0) and (dpts[pid] > 0):
                        logging.debug("lastdpts %s, dpts %s",lastdpts[pid],dpts[pid])
                        constantStream[pid] = False
                    
                    if mindpts[pid] == 0:
                        mindpts[pid] = dpts[pid]
                    else:
                        mindpts[pid] = min(dpts[pid], mindpts[pid])
                    
            pesPacket[pid] = ""

            if (bs.ts_has_payload(tsPacket)):
    #             there is a buid in funciton bs.ts_payload. However, this function returns just "" so we have to do it our own
                if(not bs.ts_has_adaptation(tsPacket)):
                    pesTsPacket = tsPacket[bs.getTsHeaderSize():]
                else:
                    pesTsPacket = tsPacket[bs.getTsHeaderSize() + 1 + bs.ts_get_adaptation(tsPacket):]
                pesPacket[pid] = pesPacket[pid] + pesTsPacket
                                            
    tsStream.close()
    
    return (mindpts,constantStream)

def parseInt(data):
    '''helperfunction, that converts a String to Int.
    
    This function uses the build in Int function. The difference is that empty strings are interpreted as 0   
    '''
    if data == "":
        return 0
    else:
        return int(data)
        
def parseFloat(data):
    '''helperfunction, that converts a String to Float.
    
    This function uses the build in Float function. The difference is that empty strings are interpreted as 0   
    '''    
    if data == "":
        return 0
    else:
        return float(data)
    
     

