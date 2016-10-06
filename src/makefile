CXX=g++

CPP_FILES = $(wildcard *.cpp)
OBJ_FILES = $(patsubst %.cpp,obj/%.o,$(CPP_FILES))

INC = -I../../boost_1_60_0 -I../../libscapi/lib/ -I../../libscapi/lib/OTExtensionBristol -I../../libscapi/install/include

CPP_OPTIONS := -w -std=c++11 $(INC) -msse4.1 -mavx -pthread -maes -msse2 -mpclmul -fpermissive -fpic

LIBS =   -L../../libscapi/install/lib -L../../boost_1_60_0/stage/lib

LINKER_OPTIONS = ../../libscapi/scapi.a ../../libscapi/install/lib/libmiracl.a ../../libscapi/install/lib/libOTExtensionBristol.a ../../libscapi/install/lib/libsimpleot.a $(LIBS) -lpthread -lgmp -lcrypto -lssl -lboost_system -lntl -lboost_thread -lOTExtension -lMaliciousOTExtension -lOTExtensionBristol -ldl -lmiracl

all: dir $(OBJ_FILES) main.exe

dir: 
	mkdir -p obj/

obj/%.o: %.cpp
	$(CXX) -c $(CPP_OPTIONS) $(LINKER_OPTIONS) -o $@ $<

main.exe: $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) $(LINKER_OPTIONS) -o $@

clean:
	rm -f ./*.exe
	rm -rf ./obj
