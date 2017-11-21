cmake_minimum_required(VERSION 2.8)
project(SemiHonestBMR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -msse4.1 -mavx -pthread -maes -msse2 -mpclmul \
-fpermissive -fpic -Wno-endif-labels -Wno-conversion-null -Wno-write-strings")


INCLUDE_DIRECTORIES($ENV{HOME} $ENV{HOME}/libscapi/lib/OTExtensionBristol $ENV{HOME}/libscapi/lib/
        $ENV{HOME}/libscapi/install/include)

set(SOURCE_FILES src/BMRParty.cpp src/basicSockets.cpp src/BMR.cpp src/BMR_BGW.cpp src/BMR_BGW_aux.cpp
        src/cbitvector.cpp src/main.cpp src/main_gf_funcs.cpp src/ottmain.cpp src/secCompMultiParty.cpp src/sha1.cpp
        src/TedKrovetzAesNiWrapperC.cpp)

link_directories($ENV{HOME}/libscapi/install/lib)

add_executable(SemiHonestBMR ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(SemiHonestBMR
        $ENV{HOME}/libscapi/libscapi.a gmp
        OTExtensionBristol libjsoncpp.a
        $ENV{HOME}/libscapi/install/lib/libsimpleot.a
        libboost_system.a libboost_thread.a pthread ntl libssl.a libcrypto.a dl z)
