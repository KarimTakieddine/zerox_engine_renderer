cmake_minimum_required(VERSION 4.3.3 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20 REQUIRED)

set(PROJECT_NAME zerox_engine_renderer)
set(INCLUDE_DIR include)
set(SRC_DIR src)
set(GLAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/zerox_engine_glad")

Project(${PROJECT_NAME})

if (NOT DEFINED MEMORY_READER_DIR)
    set(MEMORY_READER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../zerox_engine/memory_reader")
endif()

if (NOT DEFINED PLATFORM_DIR)
    set(PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../zerox_engine/zerox_engine_platform")
endif()

add_subdirectory(${GLAD_DIR})

list(APPEND SOURCES
    ${SRC_DIR}/render_frame_buffer.cpp
    ${SRC_DIR}/renderer.cpp
    ${SRC_DIR}/renderer_opengl.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_package(glm 1.0 REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIR}
    "${MEMORY_READER_DIR}/${INCLUDE_DIR}"
    "${PLATFORM_DIR}/include"
    "${GLAD_DIR}/${INCLUDE_DIR}")

target_link_libraries(${PROJECT_NAME} PRIVATE GL zerox_engine_glad glm::glm)
