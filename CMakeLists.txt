cmake_minimum_required(VERSION 3.20)
project(chocolate VERSION 1.0.0)

include(GNUInstallDirs)

set(LIB_NAME ${PROJECT_NAME})


# Find locally installed dependencies. Tip: Use VCPKG for these.

find_package(glfw3 CONFIG REQUIRED)
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

FetchContent_Declare(
    HandmadeMath
    GIT_REPOSITORY https://github.com/HandmadeMath/HandmadeMath.git
    GIT_TAG 422bc588e9e8ae580f472f05e47c01a646acff38 # v2.0.0
)

if (NOT HandmadeMath_POPULATED)
    FetchContent_MakeAvailable(HandmadeMath)
endif()


# Chocolate sources
set(SRC_Chocolate
    "foundation/allocator.cpp"
    "foundation/allocator.h"
    "foundation/api_types.h"
    "foundation/engine.h"
    "foundation/log.cpp"
    "foundation/log.h"
    "vendor/stb_image.h"
    "vendor/stb_image_write.h"
    "glad/src/glad.c"
)


# Create library
add_library(${LIB_NAME} STATIC
    ${SRC_Chocolate}
)

set_target_properties(
    ${LIB_NAME}
    PROPERTIES
    OUTPUT_NAME "chocolate"
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    LINKER_LANGUAGE C
)


# Includes
target_include_directories(${LIB_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_include_directories(${LIB_NAME} SYSTEM PUBLIC ${bitsquidfoundation_SOURCE_DIR})
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/glad/include)
target_include_directories(${LIB_NAME} SYSTEM PUBLIC ${HandmadeMath_SOURCE_DIR})

if (SUPERLUMINAL)
    target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${SuperluminalAPI_INCLUDE_DIRS})
endif()

target_compile_definitions(${LIB_NAME} PRIVATE "GLFW_INCLUDE_NONE")


# Linked libraries
target_link_libraries(${LIB_NAME}
    Backward::Backward
    bitsquid-foundation
    glfw
    cjson
    imgui::imgui
)

# Compiler warnings & definitions

include(cmake/CompilerWarnings.cmake)
myproject_set_project_warnings(${PROJECT_NAME})

if (MSVC)
    if (SUPERLUMINAL)
        target_compile_definitions(${PROJECT_NAME} PRIVATE SUPERLUMINAL=1)
    endif()
endif()
