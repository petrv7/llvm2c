project(llvm2c)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST} Func.h Block.h Type.h Program.h Expr/Expr.h Expr/Expr.cpp Expr/BinaryExpr.h Expr/BinaryExpr.cpp Expr/UnaryExpr.h Expr/UnaryExpr.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -g -Wextra -pedantic -fpermissive -lboost_program_options")

FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${Boost_LIBRARIES})

find_package(LLVM REQUIRED CONFIG)
if (${LLVM_PACKAGE_VERSION} VERSION_GREATER "3.4")
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_DIR}")
  include(LLVMConfig)
endif()
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

if (LLVM_SRC_PATH)
  include_directories(${LLVM_SRC_PATH}/include)
  SET(LLVM_INCLUDE_DIRS ${LLVM_SRC_PATH}/include)
  message(STATUS "Got LLVM_SRC_PATH ${LLVM_SRC_PATH}")
endif(LLVM_SRC_PATH)
if (LLVM_BUILD_PATH)
  include_directories(${LLVM_BUILD_PATH}/include)
  link_directories(${LLVM_BUILD_PATH}/lib)
  SET(LLVM_LIBRARY_DIRS ${LLVM_BUILD_PATH}/lib)
  message(STATUS "Got LLVM_BUILD_PATH ${LLVM_BUILD_PATH}")
endif(LLVM_BUILD_PATH)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM libraries dir: ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")

add_definitions(-DHAVE_LLVM)

# Find the libraries that correspond to the LLVM components
# that we wish to use
if (${LLVM_PACKAGE_VERSION} VERSION_GREATER "3.4")
  llvm_map_components_to_libnames(llvm_libs support core irreader bitwriter linker)
else()
  llvm_map_components_to_libraries(llvm_libs support core irreader bitwriter linker)
endif()

target_link_libraries(${PROJECT_NAME} ${llvm_libs})
