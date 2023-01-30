cmake_minimum_required(VERSION 2.8.0)  

project(ctilog)

find_package(catkin REQUIRED)
find_package(Boost REQUIRED)

#Check C++11 or C++0x support
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  add_compile_options(-std=c++11)
  message(STATUS "using flag -std=c++11.")
elseif(COMPILER_SUPPORTS_CXX0X)
  add_compile_options(-std=c++0x)
  message(STATUS "using flag -std=c++0x.")
else()
  message(FATAL_ERROR
    "the compiler ${CMAKE_CXX_COMPILER} has no C++11 support. "
    "please use a different C++ compiler.")
endif()

file(GLOB_RECURSE ALL_LIBRARY_SRCS "src/[a-zA-Z]*.c" "src/[a-zA-Z]*.cc" "src/[a-zA-Z]*.cpp")

catkin_package(
   INCLUDE_DIRS include
   LIBRARIES ${PROJECT_NAME}
   CATKIN_DEPENDS ${catkin_LIBRARIES}
   DEPENDS ${Boost_LIBRARIES}
)


include_directories(
  include
  ${catkin_INCLUDE_DIRS} 
  ${Boost_INCLUDE_DIRS}
)

add_library(${PROJECT_NAME} SHARED ${ALL_LIBRARY_SRCS})

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${catkin_LIBRARIES})

install(TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY include/
  DESTINATION ${CATKIN_GLOBAL_INCLUDE_DESTINATION}
  FILES_MATCHING PATTERN "*.hpp"
  PATTERN ".svn" EXCLUDE
)
