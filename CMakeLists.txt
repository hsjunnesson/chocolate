cmake_minimum_required(VERSION 3.7)
project(chocolate VERSION 1.0.0)

include(GNUInstallDirs)

set(LIB_NAME ${PROJECT_NAME})


# Find locally installed dependencies. Tip: Use VCPKG for these.

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)


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


# Generated protobuf implementation

file(GLOB_RECURSE SRC_Generated
    "generated_src/*.cc"
    "generated_src/*.h"
)


# Chocholate sources
file(GLOB_RECURSE SRC_Chocolate
    "src/*.c"
    "src/*.cpp"
    "glad/src/glad.c"
)

list(FILTER SRC_Chocolate EXCLUDE REGEX ".*/main.cpp$")

file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
    "include/*.inl"
)


# Create library
add_library(${LIB_NAME} STATIC
    ${SRC_Chocolate}
    ${SRC_Generated}
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
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/generated_src)
target_include_directories(${LIB_NAME} SYSTEM PUBLIC ${bitsquidfoundation_SOURCE_DIR})
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${backward_SOURCE_DIR})
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${date_SOURCE_DIR})
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/glad/include)
target_include_directories(${LIB_NAME} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/generated_src)

target_compile_definitions(${LIB_NAME} PRIVATE "GLFW_INCLUDE_NONE")


# Linked libraries
target_link_libraries(${LIB_NAME}
    backward
    bitsquid-foundation
    glfw
    glm::glm
    protobuf::libprotobuf
    date::date
)

#export(TARGETS libchocolate NAMESPACE engine:: FILE LibLicensingTargets.cmake)


# # Test application
# SET(APP_EXE StaticTest)

# add_executable(${APP_EXE}
#     "src/main.cpp"
# )


# # Includes
# include_directories(
#         ${PROJECT_SOURCE_DIR}/include
# )

# target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${bitsquidfoundation_SOURCE_DIR})
# target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${backward_SOURCE_DIR})
# target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${date_SOURCE_DIR})
# target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
# target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/generated_src)


# # Linked libraries
# target_link_libraries(${APP_EXE} PRIVATE backward)
# target_link_libraries(${APP_EXE} PRIVATE bitsquid-foundation)
# target_link_libraries(${APP_EXE} PRIVATE glfw)
# target_link_libraries(${APP_EXE} PRIVATE glm::glm)
# target_link_libraries(${APP_EXE} PRIVATE protobuf::libprotobuf)
# target_link_libraries(${APP_EXE} PRIVATE date::date)

# target_compile_definitions(${APP_EXE} PRIVATE "GLFW_INCLUDE_NONE")


# # Compiler warnings
# target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror -Wno-unknown-pragmas -Wno-gnu-zero-variadic-macro-arguments)
