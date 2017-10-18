cmake_minimum_required(VERSION 2.8)
project(SemiHonestBMR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -msse4.1 -mavx -pthread -maes -msse2 -mpclmul -fpermissive -fpic -flto" )

SET (BOOST_ROOT $ENV{HOME}/boost_1_64_0/)
SET (BOOST_INCLUDEDIR $ENV{HOME}/boost_1_64_0/)
SET (BOOST_LIBRARYDIR $ENV{HOME}/boost_1_64_0/stage/lib)
find_package(Boost COMPONENTS system thread REQUIRED)

INCLUDE_DIRECTORIES( $ENV{HOME} ${BOOST_INCLUDEDIR} $ENV{HOME}/libscapi/lib/OTExtensionBristol $ENV{HOME}/libscapi/lib/)

set(SOURCE_FILES src/BMRParty.cpp src/aes_core.cpp src/basicSockets.cpp src/BMR.cpp src/BMR_BGW.cpp src/BMR_BGW_aux.cpp
        src/cbitvector.cpp src/main.cpp src/main_gf_funcs.cpp src/ottmain.cpp src/secCompMultiParty.cpp src/sha1.cpp
        src/TedKrovetzAesNiWrapperC.cpp)


link_directories(/usr/ssl/lib/ $ENV{HOME}/libscapi/install/lib ${BOOST_LIBRARYDIR})


add_executable(SemiHonestBMR ${SOURCE_FILES})


TARGET_LINK_LIBRARIES(SemiHonestBMR
        $ENV{HOME}/libscapi/libscapi.a gmp
        OTExtensionBristol
        $ENV{HOME}/libscapi/install/lib/libsimpleot.a
        boost_system boost_thread pthread ntl crypto dl ssl z )