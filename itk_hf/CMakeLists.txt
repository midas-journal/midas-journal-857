cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Set build type.")

project(itk_hf)

add_subdirectory(hf)

set(SRCS main.cpp pipeline.cpp image.cpp cpu_kernel.cpp)

find_package(ITK)
if(ITK_FOUND)
  include(${ITK_USE_FILE})
else(ITK_FOUND)
  message(FATAL_ERROR
          "Cannot build ITKApps without ITK.  Please set ITK_DIR.")
endif(ITK_FOUND)

set(USE_QT ON CACHE BOOL "Build the application in GUI mode.")
if (USE_QT)
   find_package(Qt4 REQUIRED)
   include(${QT_USE_FILE})
   QT4_WRAP_CPP(QT_MOC_SRCS QEngineStatus.h QImageViewer.h)
   set(SRCS ${SRCS} QImageViewer.cpp QEngineStatus.cpp)
   add_definitions(-DUSE_QT=1)
else (USE_QT)
   add_definitions(-DUSE_QT=0)
endif (USE_QT)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/hf)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../api/src)
ADD_EXECUTABLE(itk_hf ${SRCS} ${QT_MOC_SRCS})
target_link_libraries(itk_hf ${ITK_LIBRARIES} ${QT_LIBRARIES} hf png)
