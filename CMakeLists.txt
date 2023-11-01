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

link_directories(${PROJECT_NAME}
        ${PROJECT_SOURCE_DIR}/lib/cross_sockets
        ${PROJECT_SOURCE_DIR}/lib/soundio)

FILE(GLOB INDEPENDENT_SRC src/*.c)

add_executable(${PROJECT_NAME} ${INDEPENDENT_SRC})

if (TARGET_IS_WINDOWS)
    target_link_libraries(${PROJECT_NAME} PRIVATE cross_sockets soundio)
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE
            m
            cross_sockets
            soundio)
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/include)