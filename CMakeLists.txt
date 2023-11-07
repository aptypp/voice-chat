cmake_minimum_required(VERSION 3.27)
project(voice_chat C)

set(CLIENT_NAME voice_chat_client)
set(SERVER_NAME voice_chat_server)

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

link_directories(${CLIENT_NAME}
        ${PROJECT_SOURCE_DIR}/lib/cross_sockets
        ${PROJECT_SOURCE_DIR}/lib/logger)

FILE(GLOB CLIENT_SRC src/client/*)
FILE(GLOB SERVER_SRC src/server/*)
FILE(GLOB SHARED_SRC src/shared/*)

#START CLIENT

add_executable(${CLIENT_NAME} ${CLIENT_SRC} ${SHARED_SRC}
        include/miniaudio.h)

if (TARGET_IS_WINDOWS)
    target_link_libraries(${CLIENT_NAME} PRIVATE
            cross_sockets
            logger.a)
else ()
    target_link_libraries(${CLIENT_NAME} PRIVATE
            m
            libcross_sockets.a
            logger.a
            pulse)
endif ()

target_include_directories(${CLIENT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)

#END CLIENT

#START SERVER

add_executable(${SERVER_NAME} ${SERVER_SRC} ${SHARED_SRC})

if (TARGET_IS_WINDOWS)
    target_link_libraries(${SERVER_NAME} PRIVATE
            libcross_sockets.a
            logger.a)
else ()
    target_link_libraries(${SERVER_NAME} PRIVATE
            m
            libcross_sockets.a
            logger.a)
endif ()

target_include_directories(${SERVER_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)

#END SERVER