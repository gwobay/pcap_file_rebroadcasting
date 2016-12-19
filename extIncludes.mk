
# C++ STD
OS_M=$(OS)
ARCH_M=$(ARCH)
STD_LOCATION=/tools/gcc/$(OS_M)/$(ARCH_M)/5.3.0
STD_LOCATION=/opt/gcc/5.3.0
LDPATH   += $(STD_LOCATION)/lib64
LDLIBS += -lstdc++ -lrt

LDPATH   += /usr/lib64/atlas
LDLIBS += -lsatlas

#CXXFLAGS += -isystem /usr/include/python2.7/

# Boost 
#BOOS_MT_FULL_VERSION=1_53_0
BOOS_MT_FULL_VERSION=1_59_0_gcc5.3.0
BOOS_MT_CENTOS_M=centos
BOOS_M_VERSION=boost_1_59_0
CENTOS_M_MACHINE_TYPE=x_64
#BOOS_MT_BASEDIR=/tools/boost/$(OS_M)/$(ARCH_M)/$(BOOS_MT_FULL_VERSION)
BOOS_MT_BASEDIR=/tools/boost/$(BOOS_MT_CENTOS_M)/$(CENTOS_M_MACHINE_TYPE)/$(BOOS_M_VERSION)
CXXFLAGS += -isystem $(BOOS_MT_BASEDIR)/include
#LDPATH   += /home/tom/lib/boost   ## debugging build of boost
LDPATH   += $(BOOS_MT_BASEDIR)/lib
LDLIBS   += -lboost_iostreams -lboost_filesystem -lboost_system -lboost_thread -lboost_date_time

# TBB
#TBB_VERSION=21_20080605oss
TBB_VERSION=4.4_u3
#TBB_VERSION=2.1
#TBB_BASEDIR=/tools/tbb/$(OS_M)/$(ARCH_M)/$(TBB_VERSION)
#TBB_BASEDIR=/tools/intel/tbb/$(OS_M)/$(ARCH_M)/$(TBB_VERSION)
TBB_BASEDIR=/tools/intel/tbb/$(TBB_VERSION)
CXXFLAGS += -isystem $(TBB_BASEDIR)/include
CPU = intel64
GCC4 = gcc4.4
LDPATH  += $(TBB_BASEDIR)/lib/$(CPU)/$(GCC4)
LDLIBS   += -ltbb

# LIB XML2
#CXXFLAGS += -isystem /usr/include/libxml2
#LDLIBS   += -lxml2

# LUA
#LUA_VERSION = 5.1.4
#LUA_BASEDIR = /tools/lua/$(OS_M)/$(ARCH_M)/$(LUA_VERSION)
LUA_VERSION = LuaJIT-2.0.4
LUA_BASEDIR = /tools/luajit/$(OS_M)/$(ARCH_M)/$(LUA_VERSION)
CXXFLAGS += -isystem $(LUA_BASEDIR)/include/luajit-2.0
LDPATH   += $(LUA_BASEDIR)/lib 
LDLIBS   += -llua -ldl

# LZO
LZO_VERSION = 2.09
#LZO_BASEDIR = /tools/lzo/$(OS_M)/$(ARCH_M)/$(LZO_VERSION)
LZO_BASEDIR=/tools/lzo/$(BOOS_MT_CENTOS_M)/$(CENTOS_M_MACHINE_TYPE)/$(LZO_VERSION)
CXXFLAGS += -isystem $(LZO_BASEDIR)/include
#LDPATH   += $(LZO_BASEDIR)/lib 
LDPATH   += $(LZO_BASEDIR)/lib
LDLIBS   += -llzo2

#ZMQ
ZMQ_VERSION = 4.2
ZMQ_BASEDIR=/tools/libzmq/$(BOOS_MT_CENTOS_M)/$(CENTOS_M_MACHINE_TYPE)/$(ZMQ_VERSION)
CXXFLAGS += -isystem $(ZMQ_BASEDIR)/include
LDPATH   += $(ZMQ_BASEDIR)/lib
LDLIBS   += -lzmq

#SNAPPY
#SNAPPY_BASEDIR = /tools/snappy/redhat/x_64/1.1.3_gcc5.3.0
SNAPPY_VERSION = 1.1.3-9
SNAPPY_BASEDIR=/tools/snappy/$(BOOS_MT_CENTOS_M)/$(CENTOS_M_MACHINE_TYPE)/$(SNAPPY_VERSION)
CXXFLAGS += -isystem $(SNAPPY_BASEDIR)/include
LDPATH   += $(SNAPPY_BASEDIR)/lib
LDLIBS   += -lsnappy

LEVELDB_BASEDIR = /tools/leveldb/redhat/x_64/1.18
CXXFLAGS += -isystem $(LEVELDB_BASEDIR)/include
LDPATH   += $(LEVELDB_BASEDIR)/lib
LDLIBS   += -lleveldb

RAPIDJSON_BASEDIR = /tools/rapidjson/any/any/0.12
CXXFLAGS += -isystem $(RAPIDJSON_BASEDIR)/include

ifdef NEED_DBL
DBL_VERSION=1.0.0
DBL_BASEDIR=/tools/dbl/$(OS_M)/$(ARCH_M)/$(DBL_VERSION)
DBL_LIBDIR=$(DBL_BASEDIR)/lib
CXXFLAGS += -isystem $(DBL_BASEDIR)/include
LDPATH  += $(DBL_LIBDIR)
endif

ifdef NEED_ONLOAD
ONLOAD_VERSION=201210
ONLOAD_BASEDIR=/tools/onload/$(OS_M)/$(ARCH_M)/$(ONLOAD_VERSION)
ONLOAD_LIBDIR=$(ONLOAD_BASEDIR)/lib
CXXFLAGS += -isystem $(ONLOAD_BASEDIR)/include
LDPATH += $(ONLOAD_LIBDIR)
LIBS   += -lonload_ext
endif

ifdef NEED_LAPACK
CXXFLAGS += -isystem /tools/atlas/$(OS_M)/$(ARCH_M)/3.10.1/include
LDPATH += /tools/atlas/$(OS_M)/$(ARCH_M)/3.10.1/lib
#LDPATH += /usr/lib/gcc/x86_64-redhat-linux/3.4.6   #needed for libg2c, compile with newer gfortran to remove dependency
#LDPATH += /usr/lib64
endif

ifdef NEED_SQLITE
CXXFLAGS += -isystem /tools/sqlite/$(OS_M)/$(ARCH_M)/3.7.8/include
LDPATH   += /tools/sqlite/$(OS_M)/$(ARCH_M)/3.7.8/lib
LDLIBS   += -lsqlite3
endif

ifdef NEED_POS_MTGRES
CXXFLAGS += -isystem /tools/postgresql/$(OS_M)/$(ARCH_M)/9.1.1/include
LDPATH   += /tools/postgresql/$(OS_M)/$(ARCH_M)/9.1.1/lib
#LDLIBS += -lpq
endif

ifdef NEED_SOCI
CXXFLAGS += -isystem /tools/soci/$(OS_M)/$(ARCH_M)/3.1.0/include/soci
LDPATH   += /tools/soci/$(OS_M)/$(ARCH_M)/3.1.0/lib64
endif

ifdef NEED_ZEROMQ
ZEROMQ_VERSION = 2.2.0
CXXFLAGS += -isystem /tools/zeromq/$(OS_M)/$(ARCH_M)/$(ZEROMQ_VERSION)/include
LDPATH   += /tools/zeromq/$(OS_M)/$(ARCH_M)/$(ZEROMQ_VERSION)/lib
LDLIBS   += -lzmq
endif

ifdef NEED_RFA
CXXFLAGS += -isystem /tools/reuters/redhat/x_64/upa7.6.1/Include
CXXFLAGS += -isystem /tools/reuters/redhat/x_64/upa7.6.1/ValueAdd/Include
LDPATH   += /tools/reuters/redhat/x_64/upa7.6.1/Libs/RHEL6_64_GCC444/Optimized
LDPATH   += /tools/reuters/redhat/x_64/upa7.6.1/Libs/RHEL6_64_GCC444/Optimized/Shared
LDPATH   += /tools/reuters/redhat/x_64/upa7.6.1/ValueAdd/Libs/RHEL6_64_GCC444/Optimized
LDPATH   += /tools/reuters/redhat/x_64/upa7.6.1/ValueAdd/Libs/RHEL6_64_GCC444/Optimized/Shared
endif

# These are obtained through R
# 
R_VERSION  = 3.2.2
R_PACKAGES = /q/common/exec/R/packages/$(R_VERSION)/20150921
R_CXXFLAGS = -isystem /tools/R/redhat/x_64/$(R_VERSION)/lib64/R/include/ -isystem $(R_PACKAGES)/Rcpp/include -isystem $(R_PACKAGES)/RInside/include
R_LDFLAGS = -Wl,--export-dynamic -fopenmp -L/tools/R/redhat/x_64/$(R_VERSION)/lib64/R/lib -Wl,-rpath=/tools/R/redhat/x_64/$(R_VERSION)/lib64/R/lib -lR -lRblas -lRlapack -lrt -ldl -lm -fopenmp -m64 -L/opt/intel/composer_xe_2015.0.090/mkl/lib/intel64 -Wl,-rpath=/opt/intel/composer_xe_2015.0.090/mkl/lib/intel64 -Wl,-rpath=/opt/intel/compilers_and_libraries_2016.0.109/linux/compiler/lib/intel64 -lmkl_intel_lp64 -lmkl_core -lmkl_intel_thread -lpthread -lm -L$(R_PACKAGES)/RInside/lib -lRInside -Wl,-rpath,$(R_PACKAGES)/RInside/lib

