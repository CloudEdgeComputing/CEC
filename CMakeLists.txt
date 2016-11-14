cmake_minimum_required(VERSION 2.6)
project(cec_mig_framwork)

add_executable(cec_mig_framwork STATEMANAGER.cpp DESTCELL.cpp SRCCELL.cpp PACKET.cpp UNIONCELL.cpp CELL.cpp Debug.cpp MIGRATION.cpp main.cpp BASICCELL.cpp TUPLE.cpp STREAMFACTORY.cpp FACTORYBUILDER.cpp WORKER.cpp PIPE.cpp Functions.cpp)

link_directories(/usr/lib/x86_64-linux-gnu/)

find_package(Boost 1.45.0 COMPONENTS Any) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})  
    target_link_libraries(cec_mig_framwork ${Boost_LIBRARIES})
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME ON) 

target_link_libraries(cec_mig_framwork libmysqlclient.so)


install(TARGETS cec_mig_framwork RUNTIME DESTINATION bin)

include_directories(/usr/local/include/)
include_directories(include)

add_subdirectory(include)