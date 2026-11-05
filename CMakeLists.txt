cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

project(CPSCap VERSION 1.0.0)

# Add all source files inside src (recursively)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Dear ImGui for cocos2d-x (MIT, matcool/gd-imgui-cocos), drawn on top of the game.
CPMAddPackage(
    NAME imgui-cocos
    GITHUB_REPOSITORY matcool/gd-imgui-cocos
    GIT_TAG b93f08ccef778a53ebba09b20c347f6a63980119
    OPTIONS "IMGUI_HOOK_EARLY OFF"
)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX IMGUI_DEFINE_MATH_OPERATORS)
target_link_libraries(${PROJECT_NAME} imgui-cocos)

# Set up dependencies, resources, and link Geode.
setup_geode_mod(${PROJECT_NAME})