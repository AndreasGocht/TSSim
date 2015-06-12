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

This module hold the Class to manage the Stream Database.
It could be used to add files to the db, dump the db to a json file,
or load the deb from a .json file.
Morover it supports a few db request to build a configuration for the simulation.
For Details see the "configGet*" functions.
'''

import sqlite3
import json
import logging
import MediaInfoDLL as me
import index_helper_functions as ihf

class StreamDatabase(object):
    '''
    Class to manage the Database
    '''

    def __init__(self, dbname):
        '''establish the Connection to the Database
        
        This function create the object and etablish the connection to the Database dbname
        
        dbname -- name of the database. .db is appended to find it in the filesystem.
        '''
        
        self.conn = sqlite3.connect(dbname + '.db')
        self.conn.row_factory = sqlite3.Row
        self.c = self.conn.cursor()
        self.dbname = dbname
        
    def new(self):
        '''create tables in an empty database
        '''
        self.c.execute('''CREATE TABLE stream (id INTEGER PRIMARY KEY AUTOINCREMENT, stream, overallBitRate REAL, overallBitRateMode, format, duration INTEGER, UNIQUE (stream) ON CONFLICT REPLACE);''')
        self.c.execute('''CREATE TABLE program (streamId INTEGER, programId INTEGER, programPid INTEGER, pcrPid INTEGER, programName , programProvider, UNIQUE (streamId, programID) ON CONFLICT REPLACE);''')
        self.c.execute('''CREATE TABLE tsstream (streamId, programId, pid, type, pcr BOOLEAN, middleBitrate REAL, maxBitrate REAL, format, width INTEGER, height INTEGER, frameRate REAL, bitStreamFormat, mindPts INTEGER, constantdPts BOOLEAN, UNIQUE (streamId, pid) ON CONFLICT REPLACE);''')
        self.conn.commit()

    def check(self):
        '''check if database is created
        '''
        stream = False
        tsStream = False
        program = False
        
        self.c.execute('''
            SELECT name 
            FROM sqlite_master 
            WHERE type='table' AND name='stream';''')
        if len(self.c.fetchall()) == 1:
            stream = True

        self.c.execute('''
            SELECT name 
            FROM sqlite_master 
            WHERE type='table' AND name='tsstream';''')
        if len(self.c.fetchall()) == 1:
            tsStream = True
        
        self.c.execute('''
            SELECT name 
            FROM sqlite_master 
            WHERE type='table' AND name='program';''')
        if len(self.c.fetchall()) == 1:
            program = True
            
        return (stream and tsStream and program) 
        
            
        

    def printData(self):
        ''' print the contend of the database
        '''
        print "main:"
        for row in self.c.execute('SELECT * FROM stream'):
            print row            
        print "program:"
        for row in self.c.execute('SELECT * FROM program'):
            print row
        print "video:"
        for row in self.c.execute('SELECT * FROM tsStream'):
            print row


    def clear(self):
        ''' delete the tabels of the database
        '''
        self.c.execute('DROP TABLE stream')
        self.c.execute('DROP TABLE program')
        self.c.execute('DROP TABLE tsStream')
        self.conn.commit()
    

    def parseMenuId(self, p, streamType, i):
        ''' helperfunction
        
        Sometimes it happeds that a pid has more than one program entry, and seems to belong to more than one Channel.
        However, this function ensures that a database compatible formate is used and return just the first programId.
        
        p -- MediaInfo Object
        streamType -- streamType (me.Stream.Video, me.Stream.Audio)
        i -- index of the stream 
        '''
        
        program = p.Get(streamType, i, "MenuID")
        if not program.find("/") == -1:
            logging.warn("tow Menus found. Chossing just the first one")
            program = program[:program.find("/")]
            program = program.replace(" ", "")
        programId = ihf.parseInt(program)
        return programId

    def getStreamId(self, filename):
        ''' helperfunction
        
        Function that gets the internal steram id to a saved filename.
        '''
        for row in self.c.execute('SELECT id FROM stream WHERE stream = ?', (filename, )):
            return row

    def add(self, filename):
        '''add the a stream to the database
        
        This function parses the info provided by MediaInfo. When MediaInfo provides no Information
        index_helper_functions is used to get theas Information about the stream. 
        
        filename -- path to the stream to add
        '''
        logging.debug("start parsing")
        p = me.MediaInfo()
        p.Open(filename)
        
        id = None
        overAllBitRate = None
        
        if not ihf.parseInt(p.Get(me.Stream.General, 0, "StreamCount")) == 1:
            logging.warn("Trouble parsing stream %s, to much Gerneral Info. Using just the first one", filename)
            
        type = "General"
        format = p.Get(me.Stream.General, 0, "Format")
        
        overAllBitRateMode = p.Get(me.Stream.General, 0, "OverallBitRate_Mode/String")
        overAllBitRate = ihf.parseFloat(p.Get(me.Stream.General, 0, "OverallBitRate"))
        duration = ihf.parseFloat(p.Get(me.Stream.General, 0, "Duration"))/1000
        if (overAllBitRate == 0) or (duration == 0):
            logging.debug("No MediaInfo Bitrate, recalculating BitRate")
            newInfo = ihf.calculateBitrateAndDuration(filename)
            overAllBitRate = newInfo["bitrate"]
            duration = newInfo["duration"]/1000
        
        self.c.execute('INSERT INTO stream(stream, overAllBitRate,overallBitRateMode, format, duration) VALUES (?,?,?,?,?)'\
                       ,(filename, overAllBitRate, overAllBitRateMode, format, duration))
        
        id = self.getStreamId(filename)[0]
        
        #get and write the basic informations

        programs = ihf.findAndParsePAT(filename)
        pmtInfo = ihf.findAndParsePMTs(filename, programs)

        #calculate all bitrates        
        data = ihf.calculateMaxBitrate(filename, overAllBitRate, 1)
        
        #pars pats and pmt and save their informations
        for programPid in pmtInfo:
            programId = programs[programPid]
            pcrPid = pmtInfo[programPid]["pcrPid"]            
            self.c.execute('''REPLACE INTO program(streamId, programId, programPid, pcrPid) 
                VALUES (?, ?, ?, ?)'''\
                , (id, programId, programPid, pcrPid))
            if not pcrPid in pmtInfo[programPid]["pids"]:
                try:
                    middleBitrate = (data[pcrPid]["countTotal"] * 188 * 8) / (duration)
                    maxBitrate = data[pcrPid]["maxBitRate"]
                except KeyError:
                    middleBitrate = 0
                    maxBitrate = 0
                    logging.error("KeyError for pid %i Set middleBitrate, maxBitrate = 0", pcrPid)

                self.c.execute('''REPLACE INTO tsStream(streamId, programId, pid, pcr, type, middleBitrate, maxBitrate)
                        VALUES (?, ?, ?, 1, "pcrPid", ?, ?)'''
                        , (id, programId, pcrPid, middleBitrate, maxBitrate))
            
            for pid in pmtInfo[programPid]["pids"]:
                try:
                    middleBitrate = (data[pid]["countTotal"] * 188 * 8) / (duration)
                    maxBitrate = data[pid]["maxBitRate"]
                except KeyError:
                    middleBitrate = 0
                    maxBitrate = 0
                    logging.error("KeyError for pid %i Set middleBitrate, maxBitrate = 0", pid)
                    
                if pid == pcrPid:
                    pcr = True
                    type = "pcrPid"
                else:
                    pcr = False
                    type = "unknown"
                self.c.execute('''REPLACE INTO tsStream(streamId, programId, pid, pcr, type, bitStreamFormat, middleBitrate, maxBitrate)
                    VALUES (?, ?, ?, ?, ?, ?, ?, ?)'''\
                    ,(id, programId, pid, pcr, type, pmtInfo[programPid]["pids"][pid], middleBitrate, maxBitrate))

        #add addtional informations from MediaInfo
        for i in range(ihf.parseInt(p.Get(me.Stream.Video, 0, "StreamCount"))):
            logging.debug("Video Found")
            type = "Video"
            pid = ihf.parseInt(p.Get(me.Stream.Video, i, "ID"))
            height = ihf.parseInt(p.Get(me.Stream.Video, i, "Height"))
            width = ihf.parseInt(p.Get(me.Stream.Video, i, "Width"))
            format = p.Get(me.Stream.Video, i, "Format")
            frameRate = ihf.parseFloat(p.Get(me.Stream.Video, i, "FrameRate"))
            self.c.execute('''UPDATE tsStream 
                SET format = ? , width = ?, height = ?, frameRate = ?, type = ?
                WHERE streamId = ? AND pid = ?'''\
                ,(format, width, height, frameRate, type,\
                  id, pid))

        for i in range(ihf.parseInt(p.Get(me.Stream.Audio, 0, "StreamCount"))):
            logging.debug("Audio Found")
            type = "Audio"            
            pid = ihf.parseInt(p.Get(me.Stream.Audio, i, "ID"))
            format = p.Get(me.Stream.Audio, i, "Format")
            (mindPts,constantdPts) = ihf.parseDeltaPtsAudio(filename, [pid])
            self.c.execute('''UPDATE tsStream
                SET type = ?, format = ?, mindPts = ?, constantdPts = ? 
                WHERE streamId = ? AND pid = ?'''\
                , (type ,format, mindPts[pid], constantdPts[pid], id, pid))
                    
        for i in range(ihf.parseInt(p.Get(me.Stream.Menu, 0, "StreamCount"))):
            type = "Menu"
            
            programId = p.Get(me.Stream.Menu, i, "MenuID")
            programName = p.Get(me.Stream.Menu, i, "ServiceName")
            programProvider = p.Get(me.Stream.Menu, i, "ServiceProvider")
            self.c.execute('''UPDATE program 
                SET programName = ? , programProvider = ? 
                WHERE programId = ? AND streamId = ?'''\
                , (programName, programProvider, programId, id))
        self.conn.commit()
        p.Close()
        
    def configGetAll(self):
        '''SQL query that returns all combinations of audio and video Pids in one program of one stream.
        
        The function might be used to simulate different audio and video combinations
        
        return -- return a dictionary:
            result["stream"] -- path to the stream
            result["overallBitRate"] -- bitRate to read the stream
            result["duration"] -- duration of the stream
            result["audioPid"] -- audio Pid for the demux
            result["videoPid"] -- video Pid for the demux
            result["pcrPid"] -- pcr Pid for the demux
            result["frameRate"] -- framerate of the stream
            result["programName"] -- name of the programm 
            result["videoBitStreamFormat"] -- string of bitstream, coding of the Videostream
            result["height"] -- height of the picture             
            result["width"] -- width of the picture
            result["mindPts"] -- minmal delta between tow audio pts
            result["constantdPts"] -- True if delta between audio pts is constant
            result["rowId"] -- number of result in the list of results
        '''
        result = []
        rowId = 0
        for row in self.c.execute('''
            SELECT stream.stream as stream, stream.id as id, stream.overallBitrate as overallBitrate
                , stream.duration as duration, audio.pid as audioPid
                , video.pid as videoPid, program.pcrPid as pcrPid
                , video.frameRate as frameRate 
                , program.programName as programName 
                , video.bitStreamFormat as videoBitStreamFormat
                , video.height as height
                , video.width as width
                , audio.mindPts as mindPts
                , audio.constantdPts as constantdPts
                FROM stream
                JOIN program ON program.streamId = stream.id
                JOIN tsstream as video ON program.programId = video.programId AND stream.id = video.streamId
                JOIN tsstream as audio ON program.programId = audio.programId AND stream.id = audio.streamId
                WHERE video.type = "Video" AND audio.type = "Audio"'''):
            d = {}
            for k in row.keys():
                d[k] = row[k]
            d["rowId"]= rowId
            result.append(d)
            rowId = rowId + 1
        return result
    
    def configGetAllConstBitrate(self):
        '''SQL query that returns all combinations of audio and video Pids in one program of one stream with a constant Bitrate.
        
        The function might be used to simulate different audio and video combinations
        
        return -- return a diconary:
            result["stream"] -- path to the stream
            result["overallBitRate"] -- bitRate to read the stream
            result["duration"] -- duration of the stream            
            result["audioPid"] -- audio Pid for the demux
            result["videoPid"] -- video Pid for the demux
            result["pcrPid"] -- pcr Pid for the demux
            result["frameRate"] -- framerate of the stream
            result["programName"] -- name of the programm
            result["videoBitStreamFormat"] -- string of bitstream, coding of the Videostream
            result["height"] -- height of the picture             
            result["width"] -- width of the picture       
            result["mindPts"] -- minmal delta between tow audio pts
            result["constantdPts"] -- True if delta between audio pts is constant
            result["rowId"] -- number of result in the list of results            
        '''
        result = []
        rowId = 0
        for row in self.c.execute('''
            SELECT stream.stream as stream, stream.id as id, stream.overallBitrate as overallBitrate
                , stream.duration as duration, audio.pid as audioPid
                , video.pid as videoPid, program.pcrPid as pcrPid
                , video.frameRate as frameRate 
                , program.programName as programName
                , video.bitStreamFormat as videoBitStreamFormat 
                , video.height as height
                , video.width as width  
                , audio.mindPts as mindPts
                , audio.constantdPts as constantdPts
                FROM stream
                JOIN program ON program.streamId = stream.id
                JOIN tsstream as video ON program.programId = video.programId AND stream.id = video.streamId
                JOIN tsstream as audio ON program.programId = audio.programId AND stream.id = audio.streamId
                WHERE video.type = "Video" AND audio.type = "Audio" AND stream.overallBitRateMode = "Constant"'''):
            d = {}
            for k in row.keys():
                d[k] = row[k]
            d["rowId"]= rowId
            result.append(d)
            rowId = rowId + 1

        return result

    def configGetFile(self,name):
        '''SQL query that returns all combinations of audio and video Pids in one program of a transport stream file.
        
        The function might be used to simulate different audio and video combinations
        
        return -- return a list containing diconarys in the form:
            result["stream"] -- path to the stream
            result["overallBitRate"] -- bitRate to read the stream
            result["duration"] -- duration of the stream            
            result["audioPid"] -- audio Pid for the demux
            result["videoPid"] -- video Pid for the demux
            result["pcrPid"] -- pcr Pid for the demux
            result["frameRate"] -- framerate of the stream
            result["programName"] -- name of the programm
            result["videoBitStreamFormat"] -- string of bitstream, coding of the Videostream
            result["height"] -- height of the picture             
            result["width"] -- width of the picture
            result["mindPts"] -- minmal delta between tow audio pts
            result["constantdPts"] -- True if delta between audio pts is constant            
            result["rowId"] -- number of result in the list of results            
        '''
        result = []
        rowId = 0
        for row in self.c.execute('''
            SELECT stream.stream as stream, stream.id as id, stream.overallBitrate as overallBitrate
                , stream.duration as duration, audio.pid as audioPid
                , video.pid as videoPid, program.pcrPid as pcrPid
                , video.frameRate as frameRate 
                , program.programName as programName
                , video.bitStreamFormat as videoBitStreamFormat
                , video.height as height
                , video.width as width
                , audio.mindPts as mindPts
                , audio.constantdPts as constantdPts                
                FROM stream
                JOIN program ON program.streamId = stream.id
                JOIN tsstream as video ON program.programId = video.programId AND stream.id = video.streamId
                JOIN tsstream as audio ON program.programId = audio.programId AND stream.id = audio.streamId
                WHERE video.type = "Video" AND audio.type = "Audio" AND stream.stream like ?''',("%" + name + "%",)):
            d = {}
            for k in row.keys():
                d[k] = row[k]
            d["rowId"]= rowId
            result.append(d)
            rowId = rowId + 1
        return result
    
    def findMenusWithoutPcr(self):
        '''Finds programs that don't contain a PCR stream
        
        It migth happend that the video stream dos not have the pcr Value. This function can be used to find this streams
        For this streams a user action is necesarry to build and run a simulation.
        
        return -- dictonary conatining the streams:
            result["stream"] -- path to the stream
            result["streamId"] -- DB internal ID
            result["programId"] -- program id
        '''
        result = []
        for row in self.c.execute('''
            SELECT stream.id as streamId, stream.stream as stream, program.programId as programId
                FROM program
                JOIN stream ON stream.id = program.streamId
                JOIN tsstream as ts1 ON program.programId = ts1.programId AND program.streamId = ts1.streamId  
                WHERE NOT EXISTS (
                    SELECT mn.streamId, mn.programId FROM program  as mn
                        JOIN tsstream as ts2 ON mn.programId = ts2.programId AND mn.streamId = ts2.streamId AND ts2.pcr = 1
                        WHERE mn.programId = program.programId AND mn.streamId = program.streamId
                    )    '''):
            result.append({"stream": row["stream"],
                           "streamId": row['streamId'],
                           "programId": row["programId"]})
        return result

            
    def saveJson(self):
        ''' This function save the database to a .json file.
        
        The filename will be the in the constructor given dbname.json
        '''
        table = {}        
        def dict_factory(cursor, row):
            d = {}
            for idx, col in enumerate(cursor.description):
                d[col[0]] = row[idx]
            return d
        
        oldFactory = self.conn.row_factory
        self.conn.row_factory = dict_factory
        c = self.conn.cursor()
        c.execute('SELECT * FROM stream')
        table['stream'] = c.fetchall()
        c.execute('SELECT * FROM program')
        table['program'] = c.fetchall()
        c.execute('SELECT * FROM tsstream')
        table['tsstream'] = c.fetchall()
        
        with open(self.dbname+".json","w") as f:
            json.dump(table,f,indent=4, separators=(',', ': '))
            
        self.conn.row_factory = oldFactory
            
            
    def loadJson(self):
        ''' This function loads the database from a .json file.
        
        The filename will be the in the constructor given dbname.json
        '''

        with open(self.dbname + ".json", "r") as f:
            table = json.load(f)
            
        self.c.executemany('REPLACE INTO stream VALUES (:id,:stream,:overallBitRate,:overallBitRateMode,:format,:duration)',table['stream'])
        self.c.executemany('REPLACE INTO program VALUES (:streamId, :programId, :programPid, :pcrPid, :programName , :programProvider)',table['program'])
        self.c.executemany('REPLACE INTO tsstream VALUES (:streamId, :programId, :pid, :type, :pcr, :middleBitrate, :maxBitrate, :format, :width, :height, :frameRate, :bitStreamFormat, :mindPts, :constantdPts)',table['tsstream'])
        
        self.conn.commit()