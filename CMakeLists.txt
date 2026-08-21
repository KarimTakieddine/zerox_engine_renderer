cmake_minimum_required(VERSION 4.3.3 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20 REQUIRED)

set(PROJECT_NAME zerox_engine_renderer)
set(INCLUDE_DIR include)
set(SRC_DIR src)
set(GLAD_LOCAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zerox_engine_glad)
set(MEMORY_READER_LOCAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/memory_reader)

Project(${PROJECT_NAME})

if (NOT DEFINED GLAD_DIR)
    set(GLAD_DIR "${GLAD_LOCAL_DIR}")
    add_subdirectory(${GLAD_DIR})
endif()

if (NOT DEFINED MEMORY_READER_DIR)
   set(MEMORY_READER_DIR "${MEMORY_READER_LOCAL_DIR}")
endif()

list(APPEND SOURCES
    ${SRC_DIR}/renderer.cpp
    ${SRC_DIR}/renderer_opengl.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_package(glm 1.0 REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIR}
    "${GLAD_DIR}/include"
    "${MEMORY_READER_DIR}/${INCLUDE_DIR}"
    "${GLAD_DIR}/${INCLUDE_DIR}")

target_link_libraries(${PROJECT_NAME} PRIVATE uring GL zerox_engine_glad glm::glm)
