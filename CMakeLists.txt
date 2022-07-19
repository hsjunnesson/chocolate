cmake_minimum_required(VERSION 3.7)
project(chocolate VERSION 1.0.0)

include(GNUInstallDirs)

set(LIB_NAME ${PROJECT_NAME})


# Find locally installed dependencies. Tip: Use VCPKG for these.

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(cJSON CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)


# Fetch dependencies from Github

include(FetchContent)

FetchContent_Declare(
    backward
    GIT_REPOSITORY https://github.com/bombela/backward-cpp.git
)

if (NOT backward_POPULATED)
    FetchContent_MakeAvailable(backward)
endif()


FetchContent_Declare(
    BitSquidFoundation
    GIT_REPOSITORY https://github.com/hsjunnesson/bitsquid-foundation.git
)

if (NOT bitsquidfoundation_POPULATED)
    FetchContent_MakeAvailable(BitSquidFoundation)
endif()


FetchContent_Declare(
    date
    GIT_REPOSITORY https://github.com/HowardHinnant/date.git
)

if (NOT date_POPULATED)
    FetchContent_MakeAvailable(date)
endif()


# Chocolate sources
file(GLOB_RECURSE SRC_Chocolate
    "src/*.c"
    "src/*.cpp"
    "src/*.inl"
    "glad/src/glad.c"
)

list(FILTER SRC_Chocolate EXCLUDE REGEX ".*/main.cpp$")

file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
    "include/*.inl"
)

file(GLOB_RECURSE SRC_Imgui
    "bindings/imgui_impl_glfw.cpp"
    "bindings/imgui_impl_opengl3.cpp"
    "bindings/imgui_impl_glfw.h"
    "bindings/imgui_impl_opengl3.h"
)


# Create library
add_library(${LIB_NAME} STATIC
    ${SRC_Chocolate}
    ${SRC_Imgui}
    ${HEADERS}
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
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${backward_SOURCE_DIR})
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${date_SOURCE_DIR})
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/glad/include)

target_compile_definitions(${LIB_NAME} PRIVATE "GLFW_INCLUDE_NONE")


# Linked libraries
target_link_libraries(${LIB_NAME}
    backward
    bitsquid-foundation
    glfw
    glm::glm
    date::date
    cjson
    imgui::imgui
)

# Enable tests

enable_testing()
add_subdirectory(tests)
