cmake_minimum_required(VERSION 2.8)
set(CMAKE_BUILD_TYPE Release CACHE STRING "Set build type.")

project(itk_edge_detect)

set(SRCS main.cpp)

find_package(ITK)
if(ITK_FOUND)
  include(${ITK_USE_FILE})
else(ITK_FOUND)
  message(FATAL_ERROR
          "Cannot build ITKApps without ITK.  Please set ITK_DIR.")
endif(ITK_FOUND)

add_executable(itk_edge_detect ${SRCS})
target_link_libraries(itk_edge_detect ${ITK_LIBRARIES})
