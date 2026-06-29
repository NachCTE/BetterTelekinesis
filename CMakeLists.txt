cmake_minimum_required(VERSION 3.21)

project(
    BetterTelekinesis
    VERSION 1.0.0
    DESCRIPTION "Advanced Telekinesis SKSE Plugin for Skyrim AE/VR"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

if(MSVC)
    add_compile_options(
        /W4
        /WX
        /permissive-
        /Zc:preprocessor
        /Zc:__cplusplus
        /EHsc
        /MP
    )
    add_compile_definitions(
        UNICODE
        _UNICODE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

# ---- Dependencies ----
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# ---- Source files ----
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# ---- Plugin target ----
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src"
)

target_precompile_headers(${PROJECT_NAME}
    PRIVATE
        "src/PCH.h"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        CommonLibSSE::CommonLibSSE
        spdlog::spdlog
)

# Expose project version to source code
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# ---- Post-build: optional deploy to Skyrim ----
# Set SKYRIM_DATA_PATH environment variable to auto-deploy
if(DEFINED ENV{SKYRIM_DATA_PATH})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "$ENV{SKYRIM_DATA_PATH}/SKSE/Plugins/${PROJECT_NAME}.dll"
        COMMENT "Deploying ${PROJECT_NAME}.dll to Skyrim Data"
    )
    message(STATUS "Post-build deploy enabled: $ENV{SKYRIM_DATA_PATH}")
endif()
