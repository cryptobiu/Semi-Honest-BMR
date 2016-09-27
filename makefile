#g++ -O3 -w -o OTExtnsion.out -pthread OTExtnsion/*.cpp util/*.cpp ot/*.cpp  -I 'util/Miracl/' -I 'util/' -std=c++11 -pthread -L./ -lmiracl -msse4.1 -pthread -maes -msse2 -mpclmul -fpermissive -fpic

CXX=g++
SHARED_LIB_EXT:=.so
INCLUDE_ARCHIVES_START = -Wl,-whole-archive # linking options, we prefer our generated shared object will be self-contained.
INCLUDE_ARCHIVES_END = -Wl,-no-whole-archive
SHARED_LIB_OPT:=-shared

CPP_FILES     := $(wildcard OTExtension/*.cpp)
CPP_FILES     += $(wildcard util/*.cpp)
CPP_FILES     += $(wildcard ot/*.cpp)
OBJ_FILES     := $(patsubst OTExtnsion/%.cpp,obj/%.o,$(CPP_FILES))
OBJ_FILES     += $(patsubst util/%.c,obj/%.o,$(CPP_FILES))
OBJ_FILES     += $(patsubst ot/%.c,obj/%.o,$(CPP_FILES))
OUT_DIR        = obj obj/OTExtnsion obj/util obj/ot
INC            = -I../boost_1_60_0 -I../libscapi/lib/ -I../libscapi/lib/OTExtensionBristol -I../libscapi/install/include -Iutil/Miracl -Iutil
CPP_OPTIONS   := -std=c++11 $(INC) -msse4.1 -pthread -maes -msse2 -mpclmul -fpermissive -fpic
$(COMPILE.cpp) = g++ -c $(CPP_OPTIONS) -o $@ $<
LINKER_OPTIONS = $(INCLUDE_ARCHIVES_START) ../libscapi/scapi.a ../libscapi/install/lib/libOTExtensionBristol.a ../libscapi/install/lib/libsimpleot.a -lpthread -lgmp -lcrypto -lssl -lboost_system -lntl \
-lboost_thread -lOTExtension -lMaliciousOTExtension -lOTExtensionBristol -ldl -L./ -lmiracl -L../boost_1_60_0/stage/lib -L../install/lib \
$(INCLUDE_ARCHIVES_END)

all:: BMR
BMR:: directories $(SLib)
directories: $(OUT_DIR)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

OTExtnsion.out: $(OBJ_FILES)
	$(CXX) OTExtnsion/main.cpp -o $@ $(OBJS) $(LINKER_OPTIONS)


%.o: ot/%.cpp
	$(CXX) -c $< -o $@

%.o :util/%.cpp
	$(CXX) -c $< -o $@

%.o :OTExtnsion/%.cpp
	$(CXX) -c $< -o $@


clean:
	rm -rf obj
	rm *.a
