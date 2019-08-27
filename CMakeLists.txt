cmake_minimum_required (VERSION 3.1)
cmake_policy(VERSION 3.1)
project (CBlip)

### BUILD SETTINGS:

set(COMPILE_FLAGS   "${COMPILE_FLAGS}   -Wall -Werror")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

include_directories(
"include"
)
set(ALL_SRC_FILES 
"src/cblip.c"
"src/varint.c"
"src/ack_handler.c"
"src/msg_handler.c"
"src/hashset.c")

### LIBRARY:

if(WIN32 OR ANDROID)
    add_subdirectory("vendor/zlib")
    include_directories("vendor/zlib" "${CMAKE_CURRENT_BINARY_DIR}/vendor/zlib")
endif()

add_library(CBlip SHARED ${ALL_SRC_FILES})
if(WIN32 OR ANDROID)
    target_link_libraries(CBlip zlibstatic)
endif()

add_executable(CBlipDriver "program/main.c")
target_link_libraries(CBlipDriver CBlip)