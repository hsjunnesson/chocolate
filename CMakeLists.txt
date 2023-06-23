cmake_minimum_required(VERSION 3.21)
project(chocolate VERSION 1.0.0)

include(GNUInstallDirs)

set(LIB_NAME ${PROJECT_NAME})


# Find locally installed dependencies. Tip: Use VCPKG for these.

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(cJSON CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Backward CONFIG REQUIRED)

if (SUPERLUMINAL)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "c:/Program Files/Superluminal/Performance/API")
    find_package(SuperluminalAPI REQUIRED)
endif()


# Fetch dependencies from Github

include(FetchContent)

FetchContent_Declare(
    BitSquidFoundation
    GIT_REPOSITORY https://github.com/hsjunnesson/bitsquid-foundation.git
)

if (NOT bitsquidfoundation_POPULATED)
    FetchContent_MakeAvailable(BitSquidFoundation)
endif()


# Chocolate sources
set(SRC_Chocolate
    "src/action_binds.cpp"
    "src/atlas.cpp"
    "src/canvas.cpp"
    "src/config.cpp"
    "src/engine.cpp"
    "src/file.cpp"
    "src/input.cpp"
    "src/log.cpp"
    "src/shader.cpp"
    "src/sprites.cpp"
    "src/texture.cpp"
    "glad/src/glad.c"
)

set(HEADERS_Chocolate
    "engine/action_binds.h"
    "engine/atlas.h"
    "engine/canvas.h"
    "engine/color.inl"
    "engine/config.h"
    "engine/engine.h"
    "engine/file.h"
    "engine/ini.h"
    "engine/input.h"
    "engine/log.h"
    "engine/math.inl"
    "engine/shader.h"
    "engine/sprites.h"
    "engine/stb_image.h"
    "engine/texture.h"
)


# Create library
add_library(${LIB_NAME} STATIC
    ${SRC_Chocolate}
    ${HEADERS_Chocolate}
)

set_target_properties(
    ${LIB_NAME}
    PROPERTIES
    OUTPUT_NAME "chocolate"
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    LINKER_LANGUAGE CXX
)


# Includes
target_include_directories(${LIB_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(${LIB_NAME} SYSTEM PUBLIC ${bitsquidfoundation_SOURCE_DIR})
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/glad/include)

if (SUPERLUMINAL)
    target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${SuperluminalAPI_INCLUDE_DIRS})
endif()

target_compile_definitions(${LIB_NAME} PRIVATE "GLFW_INCLUDE_NONE")


# Linked libraries
target_link_libraries(${LIB_NAME}
    Backward::Backward
    bitsquid-foundation
    glfw
    glm::glm
    cjson
    imgui::imgui
)

if (SUPERLUMINAL)
    target_link_libraries(${LIB_NAME} SuperluminalAPI)
endif()


# Compiler warnings & definitions

include(cmake/CompilerWarnings.cmake)
myproject_set_project_warnings(${PROJECT_NAME})

if (MSVC)
    if (SUPERLUMINAL)
        target_compile_definitions(${PROJECT_NAME} PRIVATE SUPERLUMINAL=1)
    endif()
endif()


# Enable tests

enable_testing()
add_subdirectory(tests)
