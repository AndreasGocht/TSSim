CWD = $(shell pwd)
SYSTEMC_DIR = $(CWD)/systemc-2.3.1
SYSTEMC_DIR_OBJ = $(SYSTEMC_DIR)/objdir

BITSTERAMER_DIR = $(CWD)/bitstream
RAPIDJSON_DIR = $(CWD)/rapidjson

SIMULATOR_BUILD = $(CWD)/build

TESTSUITE_DIR = $(CWD)/testsuite
TESTSUITE_RESULT_DIR = $(TESTSUITE_DIR)/results
TESTSUITE_HELPERFUNCTIONS = $(TESTSUITE_DIR)/helper_functions
TESTSUITE_HELPERFUNCTIONS_BITSTREAM = $(TESTSUITE_HELPERFUNCTIONS)/bitstream

STREAMS_DIR = $(CWD)/streams

PWD := $(shell pwd)
MANIFEST := $(PWD)/manifest

.PHONY: all systemc
all : $(SIMULATOR_BUILD)/simulator $(MANIFEST)/testsuite

.PHONY: clean cleansimulator cleansystemc cleanbitstream clearrapidjson cleanStreamDatabase
clean: cleansimulator cleansystemc cleanbitstream clearrapidjson cleantestsuite cleanStreamDatabase
	rm -rf $(MANIFEST)
	
# # # # # # # # # # # 	
# print help
# # # # # # # # # # # 
	
.PHONY: help
help:
	$(info make all -- will build everything)
	$(info make testsuiteexample -- will setup and run the test pipeline)
	$(info make clean -- will clean eveything besides the systemc downlad an the content of the streams directory)

# # # # # # # # # # # 	
# create dirs
# # # # # # # # # # # 
	
$(MANIFEST):
	mkdir -p $@

$(SYSTEMC_DIR_OBJ) $(BITSTERAMER_DIR) $(SIMULATOR_BUILD) $(STREAMS_DIR):
	mkdir -p $@

# # # # # # # # # # # 	
# check, get and build dependencys
# # # # # # # # # # # 	
	
systemc-2.3.1.tgz:
	ifeq ( ,$(wildcard ./systemc-2.3.1.tgz))
		$(error Pleas download systemC 2.3.1 from http://www.accellera.org/downloads/standards/systemc/ an put in the root folder of the project)
	endif
# 	wget http://www.accellera.org/downloads/standards/systemc/accept_license/accepted_download/systemc-2.3.1.tgz	

$(MANIFEST)/extracted_systemc: systemc-2.3.1.tgz |$(MANIFEST)
	tar -xzf systemc-2.3.1.tgz 
	touch $@

$(MANIFEST)/systemc: $(MANIFEST)/extracted_systemc |$(MANIFEST) $(SYSTEMC_DIR_OBJ)
	cd $(SYSTEMC_DIR_OBJ) && ../configure --prefix=$(PWD)/local
	$(MAKE) -C $(SYSTEMC_DIR_OBJ)
	$(MAKE) -C $(SYSTEMC_DIR_OBJ) check
	$(MAKE) -C $(SYSTEMC_DIR_OBJ) install
	touch $@

systemc: $(MANIFEST)/systemc

cleansystemc:
	rm -rf $(SYSTEMC_DIR)
	rm -f $(MANIFEST)/systemc
	rm -f $(MANIFEST)/extracted_systemc

bitstream: |$(BITSTERAMER_DIR)
	git clone git://git.videolan.org/bitstream.git $(BITSTERAMER_DIR)
	
cleanbitstream:
	rm -rf $(BITSTERAMER_DIR)
	
rapidjson:
	git clone https://github.com/miloyip/rapidjson.git $(RAPIDJSON_DIR)
	touch $@

clearrapidjson:
	rm -rf $(RAPIDJSON_DIR)
	
bootstrap: bitstream rapidjson $(MANIFEST)/systemc

# # # # # # # # # # # 	
# build simulator
# # # # # # # # # # # 	

$(SIMULATOR_BUILD)/simulator: bitstream rapidjson $(MANIFEST)/systemc | $(SIMULATOR_BUILD)
	cd $(SIMULATOR_BUILD) && cmake ../
	$(MAKE) -C $(SIMULATOR_BUILD)
	
cleansimulator:
	rm -rf $(SIMULATOR_BUILD)

# # # # # # # # # # # 	
# setup test suite
# # # # # # # # # # # 		
	
$(TESTSUITE_HELPERFUNCTIONS_BITSTREAM)/libbitstream.so.1.0: bitstream
	gcc -fPIC -Dstatic='' -fkeep-inline-functions -shared -Wl,-soname,libbitstream.so.1.0 -o $(TESTSUITE_HELPERFUNCTIONS_BITSTREAM)/libbitstream.so.1.0 $(TESTSUITE_HELPERFUNCTIONS_BITSTREAM)/bitstream.c -I$(CWD)

cleanpythonlibbitstream:
	rm -f $(TESTSUITE_HELPERFUNCTIONS_BITSTREAM)/libbitstream.so.1.0
	
$(MANIFEST)/testsuite: $(SIMULATOR_BUILD)/simulator $(TESTSUITE_HELPERFUNCTIONS_BITSTREAM)/libbitstream.so.1.0
	touch $@

cleantestsuite: cleanpythonlibbitstream
	rm -rf $(TESTSUITE_RESULT_DIR)
	rm -f $(TESTSUITE_DIR)/*.pyc
	rm -f $(MANIFEST)/testsuite	

# # # # # # # # # # # 	
# get sample streams
# # # # # # # # # # # 		
	
$(STREAMS_DIR)/bbb24p_00.zip: |$(STREAMS_DIR)
	wget http://www.w6rz.net/bbb24p_00.zip -P $(STREAMS_DIR)
	
$(STREAMS_DIR)/bbb24p_00.ts: $(STREAMS_DIR)/bbb24p_00.zip |$(STREAMS_DIR)
	unzip $(STREAMS_DIR)/bbb24p_00.zip -d $(STREAMS_DIR)
	touch $@
	
$(STREAMS_DIR)/Sintel.2010.1080p.mkv: |$(STREAMS_DIR)
	wget http://ftp.nluug.nl/pub/graphics/blender/demo/movies/Sintel.2010.1080p.mkv -P $(STREAMS_DIR)
	
$(STREAMS_DIR)/sintel.ts: $(STREAMS_DIR)/Sintel.2010.1080p.mkv |$(STREAMS_DIR)
	ffmpeg -re -i $(STREAMS_DIR)/Sintel.2010.1080p.mkv -vcodec copy -acodec copy -bsf:v h264_mp4toannexb -f mpegts $@
	
$(MANIFEST)/streams: $(STREAMS_DIR)/bbb24p_00.ts $(STREAMS_DIR)/sintel.ts |$(STREAMS_DIR)
	touch $@
	
streams: $(MANIFEST)/streams

# # # # # # # # # # # 	
# build streamDatabase
# # # # # # # # # # # 	

$(TESTSUITE_DIR)/streamDatabase.db: $(MANIFEST)/streams $(MANIFEST)/testsuite
	cd $(TESTSUITE_DIR) && ./index_files.py -name=streamDatabase

$(TESTSUITE_DIR)/streamDatabase.json: $(TESTSUITE_DIR)/streamDatabase.db
	cd $(TESTSUITE_DIR) && ./index_files.py -name=streamDatabase -save

cleanStreamDatabase:
	rm -rf $(TESTSUITE_DIR)/streamDatabase.db
	rm -rf $(TESTSUITE_DIR)/streamDatabase.json
	
streamDatabase: $(TESTSUITE_DIR)/streamDatabase.db $(TESTSUITE_DIR)/streamDatabase.json

	
# # # # # # # # # # # 	
# run example tests
# # # # # # # # # # # 	
.PHONY: testsuiteexample
testsuiteexample: $(MANIFEST)/testsuite streamDatabase
	cd $(TESTSUITE_DIR) && python test_pipeline.py
	xdg-open ./testsuite/results/test_pipeline_bigbuckbunny/result.html &
	xdg-open ./testsuite/results/test_pipeline_sintel/result.html &