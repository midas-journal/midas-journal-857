cmake_minimum_required(VERSION 2.8)

set(CMAKE_BUILD_TYPE Release CACHE STRING "Set build type.")

project(hf)

set(HF_SRC
global.cpp
thread.cpp
ComputingResource.cpp
Data.cpp
ExecutionEngine.cpp
Flow.cpp
MemoryAllocator.cpp
Scheduler.cpp
TaskOrientedModule.cpp
TaskImplementation.cpp
ThreadVPE.cpp
VirtualProcessingElement.cpp
)
    
add_definitions(-D_FILE_OFSET_BITS=64)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(hf SHARED ${HF_SRC})
target_link_libraries(hf pthread)

if (UNIX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
   set_target_properties(hf PROPERTIES COMPILE_FLAGS -fPIC)
endif (UNIX)

