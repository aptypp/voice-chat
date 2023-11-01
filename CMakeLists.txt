cmake_minimum_required(VERSION 3.27)
project(voice_chat C)

set(CMAKE_C_STANDARD 23)

#START platform settings

set(CHOOSE_TARGET_PLATFORM_AUTO true)
set(TARGET_IS_WINDOWS true)

if (CHOOSE_TARGET_PLATFORM_AUTO)
    if (WIN32)
        set(TARGET_IS_WINDOWS true)
    else ()
        set(TARGET_IS_WINDOWS false)
    endif ()
endif ()

if (TARGET_IS_WINDOWS)
    message(STATUS "TARGET FOR WINDOWS")
else ()
    message(STATUS "TARGET FOR LINUX")
endif ()

#END platform settings

add_executable(app main.c)

if (TARGET_IS_WINDOWS)
    target_link_libraries(app PRIVATE ${CMAKE_SOURCE_DIR}/lib/cross_sockets/libcross_sockets.dll)
else ()
    target_link_libraries(app PRIVATE ${CMAKE_SOURCE_DIR}/lib/cross_sockets/libcross_sockets.so)
endif ()

message(${CMAKE_SOURCE_DIR}/lib/cross_sockets/libcross_sockets.so)

target_include_directories(app PUBLIC ${CMAKE_SOURCE_DIR}/include)