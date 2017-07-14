cmake_minimum_required(VERSION 2.4.0)
project (interplanet)
set(CMAKE_BUILD_TYPE Debug)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0015 NEW)

file(GLOB SOURCES *.c)

add_subdirectory(au-engine)

include_directories(${au-engine-include})

add_executable(game ${SOURCES})
target_link_libraries(game ${au-engine-libs})
file(COPY assets DESTINATION assets)
