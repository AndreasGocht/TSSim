#!/usr/bin/python
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


This script could be used to:
    create a database from a same named .json file [-json]
    create a database by parsing the streams mention in helper_functions.streams under ["hd"] and ["sd"]
    saving the database to a same named .json file [-save]
'''
import sqlite3
import logging
import sys
from helper_functions.streams import streams 
from helper_functions.stream_database import StreamDatabase 
logging.basicConfig(format='%(asctime)s:%(levelname)s:%(name)s:%(filename)s:%(message)s',filename='indexLog.log', level=logging.INFO)

loadJson = False
saveJson = False
dbname = None

def printHelp():
    print '''usage: ''' + sys.argv[0] + ''' -name=<dbname> [-json|-save]
                -name name of the db
                -json look for a dbname.json instead of reindexing the files
                -save save the db name to <dbname>.json without any indexing
                
                WARNING this script does not check if the database exists. If so it will be deleted and overwritten.'''
    exit()

for arg in sys.argv[1:]:
    if "-json" in arg:
        loadJson = True
    elif "-name=" in arg:
        dbname = arg[arg.find("=")+1:]
    elif "-save" in arg:
        saveJson = True
    elif "-h" in arg:
        printHelp() 
    else:
        logging.error("unknown Option: %s",arg)
        printHelp()
         
if dbname == None:
    printHelp()

if saveJson and loadJson:
    logging.error("don't know what to do? Saving or Loading?")
    printHelp() 
    
db = StreamDatabase(dbname)

if saveJson:
    db.saveJson()
elif loadJson:
    #create new db
    try:
        db.clear()
    except sqlite3.OperationalError as err:
        print err
    db.new()
    db.loadJson()
    
else:
    try:
        db.clear()
    except sqlite3.OperationalError as err:
        print err
    db.new()                  
                  
    for stream in streams['hd']:
        logging.info("parse file: %s",stream)
        print stream
        db.add(stream)
        
    for stream in streams['sd']:
        logging.info("parse file: %s",stream)
        print stream
        db.add(stream)
        
        

