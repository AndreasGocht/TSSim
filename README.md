# Quick Start

Get systemC:

[http://www.accellera.org/downloads/standards/systemc/](http://www.accellera.org/downloads/standards/systemc/)

Install:

    python-mediainfodll
    python-cffi
    python-cheetah

Run:

    make
    
Run the example simulation:

    make testsuiteexample
    
or

    cd ./testsuite && python -m unittest test_basicSimulation_DB.Test
    
create your own simulations:

Go to "./testsuite" and look at the existing tests. Edit them, or write your own test to simulate something.
To run them:
make sure you are in ./testsuite
type:

    python -m unittest filename.classname

or do:

    python filename

# Build

## Requirements

### systemC

Please get SystemC from the Website:

http://www.accellera.org/downloads/standards/systemc/

and save in the same Directory as this README file.
    
### python
Please make sure the following python extansions are installed:

    mediainfodll
    cffi
    cheetah
    
In Ubuntu, run:
    
    sudo apt-get install python-mediainfodll python-cffi python-cheetah

## Make
To build everything, run:

	make all

This will build systemc, download BiTStream, rapidjson and compile the simulator.

To run an example, run:
	
	make testsuiteexample

To clean up, run :
	
	make clean

The systemc tgz will remain as well as the downloaded streams.

## TestSuite

### testcases/Simulations
To set up and run the Simulations, use testcases. There is an example configuration
which can be found in ./testsuite named test_pipeline.py.

### Indexer
There is an indexer that build an Index of all available files.
To use the script you will need:

python-mediainfodll
python-cffi

To run:

    cd testsuite
    ./index_files.py -name=<dbname> [-json] [-save]

Depending on the configuration, the script will read SD and HD files
from "helper_functions/streams.py" and parse them. !!! This may take some time.

For help see 

    ./index_files.py -h 
    
##Documentation

The documentation could be found in the Wiki.
