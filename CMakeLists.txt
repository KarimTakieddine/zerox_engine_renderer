cmake_minimum_required(VERSION 4.3.3 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20 REQUIRED)

set(PROJECT_NAME zerox_engine_renderer)
set(INCLUDE_DIR include)
set(SRC_DIR src)
set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/glad)
set(MEMORY_READER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/memory_reader)

Project(${PROJECT_NAME})

list(APPEND SOURCES
    ${GLAD_DIR}/${SRC_DIR}/glad.c
    ${SRC_DIR}/renderer.cpp
    ${SRC_DIR}/renderer_opengl.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_package(glfw3 3.4 REQUIRED)
find_package(glm 1.0 REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${INCLUDE_DIR}
    "${MEMORY_READER_DIR}/${INCLUDE_DIR}"
    "${GLAD_DIR}/${INCLUDE_DIR}")

target_link_libraries(${PROJECT_NAME} PRIVATE uring glfw GL glm::glm)
