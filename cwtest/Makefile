MAKERULES=../makerules

TARGETDIR = ..

NEED_ZEROMQ=true
NEED_LAPACK=true
NEED_SQLITE=true

include $(MAKERULES)/macros.mk

CXXFLAGS += -I../md
CXXFLAGS += -I../core
CXXFLAGS += -I../tools

include sources.mk
include ../extIncludes.mk

CXXFLAGS += -DCW_DEBUG

#BIN_PY_TARGETS = qbdf_builder.py platform.py sim.py admin.py bin_print.py risk_server.py om_server.py qsub.py runSim.sh runCsv2Sqlite.sh runRoundTrip.sh gzipCmd.sh copy_results.sh md_recorder.py mdcache.py data_server.py qbdf_dump.py sim runArraySim.sh

LIB_TARGETS = hf_cwtest
#hf_core_C_PY_TARGETS = Data Admin Alert App ExchangeSession Event Event_Hub MD OMServer RiskServer Simulator Service Trading Data_Server
#hf_core_PY_TARGETS = $(module_py_sources)
hf_cwtest_ldflags = -lhf_tools -lhf_core -lzmq -llapack -lcblas -lf77blas -latlas -lgfortran $(LDLIBS)
hf_cwtest_depends = libhf_tools.$(LIBSUFFIX)
hf_cwtest_depends += libhf_core.$(LIBSUFFIX)

#BIN_TARGETS = test
#test_depends = libhf_cwtest.$(LIBSUFFIX)
#test_sources = tests/test.cpp
#test_ldflags = -lhf_cwtest -lhf_core -lhf_tools

BIN_TARGETS += use_pipe
use_pipe_depends = libhf_cwtest.$(LIBSUFFIX)
use_pipe_sources = tests/use_pipe.cpp
use_pipe_ldflags = -lhf_cwtest -lhf_core -lhf_tools

BIN_TARGETS += onload_server
onload_server_depends = libhf_cwtest.$(LIBSUFFIX)
onload_server_sources = tests/onload_server.cpp
onload_server_ldflags = -lhf_cwtest -lhf_core -lhf_tools

BIN_TARGETS += epoll_pipe
epoll_pipe_depends = libhf_cwtest.$(LIBSUFFIX)
epoll_pipe_sources = tests/epoll_pipe.cpp
epoll_pipe_ldflags = -lhf_cwtest -lhf_core -lhf_tools -lhf_md -lhf_qbt

BIN_TARGETS += firstload_pipe
firstload_pipe_depends = libhf_cwtest.$(LIBSUFFIX)
firstload_pipe_sources = tests/firstload_pipe.cpp
firstload_pipe_ldflags = -lhf_cwtest -lhf_core -lhf_tools -lhf_md -lhf_qbt

BIN_TARGETS += onload_pipe
onload_pipe_depends = libhf_cwtest.$(LIBSUFFIX)
onload_pipe_sources = tests/onload_pipe.cpp
onload_pipe_ldflags = -lhf_cwtest -lhf_core -lhf_tools -lhf_md -lhf_qbt

BIN_TARGETS += pipe_simulator
pipe_simulator_depends = libhf_cwtest.$(LIBSUFFIX)
pipe_simulator_sources = tests/pipe_simulator.cpp
pipe_simulator_ldflags = -lhf_cwtest -lhf_core -lhf_tools -lhf_md -lhf_qbt

BIN_TARGETS += tcp_server
tcp_server_depends = libhf_cwtest.$(LIBSUFFIX)
tcp_server_sources = tests/tcp_server.cpp
tcp_server_ldflags = -lhf_cwtest -lhf_core -lhf_tools

BIN_TARGETS += cap_data
CXXFLAGS += -I/usr/include/pcap
cap_data_depends = libhf_cwtest.$(LIBSUFFIX)
cap_data_sources = tests/cap_data.cpp
cap_data_ldflags = -lhf_cwtest -lhf_core -lhf_tools -lpcap

BIN_TARGETS += pcap_parser
CXXFLAGS += -I/usr/include/pcap
pcap_parser_depends = libhf_cwtest.$(LIBSUFFIX)
pcap_parser_sources = tests/pcap_parser.cpp
pcap_parser_ldflags = -lhf_cwtest -lhf_core -lhf_tools -lpcap

BIN_TARGETS += my_match
CXXFLAGS += -I/usr/include/pcap
my_match_depends = libhf_cwtest.$(LIBSUFFIX)
my_match_sources = tests/my_match.cpp
my_match_ldflags = -lhf_cwtest -lhf_core -lhf_tools -lpcap

BIN_TARGETS += cfg_to_parameter
CXXFLAGS += -I/usr/include/pcap
cfg_to_parameter_depends = libhf_cwtest.$(LIBSUFFIX)
cfg_to_parameter_sources = tests/cfg_to_parameter.cpp
cfg_to_parameter_ldflags = -lhf_cwtest -lhf_core -lhf_tools -lpcap

#BIN_TARGETS += UnitTest
#UnitTest_depends = libhf_core.$(LIBSUFFIX)
#UnitTest_ldflags = -lboost_program_options$(BOOST_SUFFIX) -lhf_core -lhf_tools ${LDLIBS}

#BIN_TARGETS += qbdf_builder
#qbdf_builder_depends = libhf_core.$(LIBSUFFIX)
#bdf_builder_sources = Data/QBDF_Builder_Main.cpp
#qbdf_builder_ldflags = -lboost_program_options$(BOOST_SUFFIX) -lhf_core -lhf_tools

#BIN_TARGETS += cvalue_archiver
#cvalue_archiver_depends = libhf_core.$(LIBSUFFIX)
#cvalue_archiver_sources = Trading/CValue_Archiver.cpp
#cvalue_archiver_ldflags = -lboost_program_options$(BOOST_SUFFIX) -lhf_core -lhf_tools $(LDLIBS)

#BIN_TARGETS += QRec_File_Merge
#QRec_File_Merge_depends = libhf_core.$(LIBSUFFIX)
#QRec_File_Merge_sources = Data/QRec_File_Merge.cpp
#QRec_File_Merge_ldflags = -lboost_program_options$(BOOST_SUFFIX) -lhf_core -lhf_tools $(LDLIBS)

#BIN_TARGETS += QRec_File_Decode
#QRec_File_Decode_depends = libhf_core.$(LIBSUFFIX)
#QRec_File_Decode_sources = Data/QRec_File_Decode.cpp
#QRec_File_Decode_ldflags = -lboost_program_options$(BOOST_SUFFIX) -lhf_core -lhf_tools $(LDLIBS)

#BIN_TARGETS += PCAP_File_Decode
#PCAP_File_Decode_depends = libhf_core.$(LIBSUFFIX)
#PCAP_File_Decode_sources = Data/PCAP_File_Decode.cpp
#PCAP_File_Decode_ldflags = -lboost_program_options$(BOOST_SUFFIX) -lhf_core -lhf_tools $(LDLIBS)

include $(MAKERULES)/rules.mk
