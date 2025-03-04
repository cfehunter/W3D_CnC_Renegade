cmake_minimum_required(VERSION 3.24)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FeatureSummary)
include(CMakeDependentOption)

# We don't support in tree builds, so help people make the right choice.
if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Building in-source is not supported! Create a build dir and remove ${CMAKE_SOURCE_DIR}/CMakeCache.txt")
endif()

project(commando LANGUAGES C CXX)

# Set where the build results will end up
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Find/Add build dependencies and stubs.
# Only 32bit Windows can make use of these APIs as the libraries shipped with the game.
if((WIN32 OR "${CMAKE_SYSTEM}" MATCHES "Windows") AND ${CMAKE_SIZEOF_VOID_P} EQUAL 4)
    add_subdirectory(Dependencies/BinkStub)
    add_subdirectory(Dependencies/MilesStub)

    #if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "12.0.8804")
        add_subdirectory(Dependencies/olddx)
    #endif()
endif()

option(RENEGADE_CLIENT "Build full game client." ON)
add_feature_info(RenegadeClient RENEGADE_CLIENT "Build Renegade game client")

option(RENEGADE_FDS "Build as headless server." ON)
add_feature_info(RenegadeHeadless RENEGADE_FDS "Build Renegade headless server")

# Do we want to build extra SDK stuff or just the game binary?
option(RENEGADE_TOOLS "Build additional tools." ON)
add_feature_info(RenegadeTools RENEGADE_TOOLS "Build Renegade Mod Tools")

include(FetchContent)
set(GS_OPENSSL FALSE)

FetchContent_Declare(
    gamespy
    GIT_REPOSITORY https://github.com/TheAssemblyArmada/GamespySDK.git
    GIT_TAG        1ad41e71b0c8796f9ed8e7432e6d50a53f8da628
)
FetchContent_MakeAvailable(gamespy)

# Add main build targets
add_subdirectory(Code)

feature_summary(WHAT ENABLED_FEATURES DESCRIPTION "Enabled features:")
feature_summary(WHAT DISABLED_FEATURES DESCRIPTION "Disabled features:")
