cmake_minimum_required(VERSION 4.0)

cmake_policy(SET CMP0091 NEW)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(EspeakSAPI VERSION 1.2.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(STATUS "Building x64 version")
else()
    message(STATUS "Building x86 version")
endif()

include(FetchContent)

set(MAIN_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(MAIN_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(MAIN_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(COMMON_COMPILE_DEFS
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
)

set(ESPEAK_NG_WARNING_SUPPRESSIONS
    /wd4005
    /wd4068
    /wd4067
    /wd4047
    /wd4024
    /wd4090
    /wd4309
)

set(ESPEAK_NG_CLANG_SUPPRESSIONS
    -Wno-implicit-function-declaration
    -Wno-deprecated-declarations
    -Wno-incompatible-pointer-types
    -Wno-pointer-sign
)

function(configure_msvc_target target_name)
    if(MSVC)
        set_property(TARGET ${target_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endfunction()

function(suppress_espeak_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /wd4309)
    endif()
endfunction()

FetchContent_Declare(
    espeak-ng
    GIT_REPOSITORY https://github.com/gozaltech/espeak-ng.git
    GIT_TAG dev
    GIT_SHALLOW TRUE
)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries (DLL)" FORCE)
set(USE_ASYNC OFF CACHE BOOL "Disable async espeak-ng" FORCE)
set(USE_MBROLA OFF CACHE BOOL "Disable MBROLA support" FORCE)
set(USE_LIBSONIC ON CACHE BOOL "Enable libsonic support" FORCE)
set(USE_LIBPCAUDIO OFF CACHE BOOL "Disable pcaudio support" FORCE)
set(USE_KLATT ON CACHE BOOL "Enable Klatt support" FORCE)
set(USE_SPEECHPLAYER ON CACHE BOOL "Enable SpeechPlayer support" FORCE)
set(EXTRA_cmn ON CACHE BOOL "Enable extra cmn language" FORCE)
set(EXTRA_ru ON CACHE BOOL "Enable extra ru language" FORCE)
set(ENABLE_TESTS OFF CACHE BOOL "Disable espeak-ng tests" FORCE)
set(ENABLE_EMOJI_DICTIONARIES OFF CACHE BOOL "Disable compiling emojies" FORCE)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(X86_DATA_PATH "${CMAKE_BINARY_DIR}/../build_x86/_deps/espeak-ng-build/espeak-ng-data")
    if(EXISTS "${X86_DATA_PATH}")
        message(STATUS "Using existing espeak-ng-data from x86 build, skipping data compilation")
        set(ESPEAK_DATA_PATH "${X86_DATA_PATH}" CACHE PATH "Path to espeak-ng data" FORCE)
    endif()
endif()

FetchContent_MakeAvailable(espeak-ng)

foreach(target_name espeak-ng espeak-ng-bin)
    if(TARGET ${target_name})
        configure_msvc_target(${target_name})
        target_compile_definitions(${target_name} PRIVATE _CRT_SECURE_NO_WARNINGS)
        if(MSVC)
            target_compile_options(${target_name} PRIVATE ${ESPEAK_NG_WARNING_SUPPRESSIONS})
            if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                target_compile_options(${target_name} PRIVATE ${ESPEAK_NG_CLANG_SUPPRESSIONS})
            endif()
        endif()
    endif()
endforeach()

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    DOWNLOAD_NO_PROGRESS TRUE
)
FetchContent_MakeAvailable(json)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MAIN_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${MAIN_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${MAIN_ARCHIVE_OUTPUT_DIRECTORY})

add_library(EspeakWrapper STATIC
    src/espeak_wrapper.cpp
)

target_include_directories(EspeakWrapper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${espeak-ng_SOURCE_DIR}/src/include
)

target_link_libraries(EspeakWrapper PUBLIC
    espeak-ng
)

target_compile_definitions(EspeakWrapper PRIVATE ${COMMON_COMPILE_DEFS})
configure_msvc_target(EspeakWrapper)
suppress_espeak_warnings(EspeakWrapper)

add_library(EspeakConfig STATIC
    src/config_manager.cpp
)

target_include_directories(EspeakConfig PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(EspeakConfig PUBLIC
    nlohmann_json::nlohmann_json
)

target_compile_definitions(EspeakConfig PRIVATE ${COMMON_COMPILE_DEFS})
configure_msvc_target(EspeakConfig)

add_library(EspeakSAPI SHARED
    src/sapi_main.cpp
    src/com.cpp
    src/registry.cpp
    src/ISpDataKeyImpl.cpp
    src/IEnumSpObjectTokensImpl.cpp
    src/ISpTTSEngineImpl.cpp
    src/voice_token.cpp
    src/espeak_sapi.def
)

target_include_directories(EspeakSAPI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${espeak-ng_SOURCE_DIR}/src/include
)

target_link_libraries(EspeakSAPI PRIVATE
    EspeakConfig
    EspeakWrapper
    ole32
    oleaut32
    advapi32
    shell32
    shlwapi
)

target_compile_definitions(EspeakSAPI PRIVATE ${COMMON_COMPILE_DEFS})
configure_msvc_target(EspeakSAPI)
suppress_espeak_warnings(EspeakSAPI)

if(MSVC)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(EspeakSAPI PRIVATE /W3 /MP)
    else()
        target_compile_options(EspeakSAPI PRIVATE /W3)
    endif()
    target_link_options(EspeakSAPI PRIVATE
        /NODEFAULTLIB:msvcrt.lib
        /NODEFAULTLIB:msvcrtd.lib
    )
endif()

if(TARGET espeak-ng)
    add_custom_command(TARGET EspeakSAPI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:espeak-ng>"
            "$<TARGET_FILE_DIR:EspeakSAPI>/"
        COMMENT "Copying espeak-ng DLL to SAPI output directory"
    )
else()
    message(WARNING "espeak-ng target not found, DLL copy will not be automatic")
endif()

add_executable(EspeakSAPIConfig WIN32
    configurator/main.cpp
    configurator/MainDialog.cpp
    configurator/ProfileDialog.cpp
    configurator/resource.rc
)

target_include_directories(EspeakSAPIConfig PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/configurator
    ${espeak-ng_SOURCE_DIR}/src/include
)

target_link_libraries(EspeakSAPIConfig PRIVATE
    EspeakConfig
    EspeakWrapper
    comctl32
    shell32
    shlwapi
)

target_compile_definitions(EspeakSAPIConfig PRIVATE ${COMMON_COMPILE_DEFS})
configure_msvc_target(EspeakSAPIConfig)
suppress_espeak_warnings(EspeakSAPIConfig)

if(MSVC)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(EspeakSAPIConfig PRIVATE /W3 /MP)
    else()
        target_compile_options(EspeakSAPIConfig PRIVATE /W3)
    endif()
endif()

if(TARGET espeak-ng)
    add_custom_command(TARGET EspeakSAPIConfig POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:espeak-ng>"
            "$<TARGET_FILE_DIR:EspeakSAPIConfig>/"
        COMMENT "Copying espeak-ng DLL to configurator output directory"
    )
endif()

install(TARGETS EspeakSAPI EspeakSAPIConfig
    RUNTIME DESTINATION "."
    LIBRARY DESTINATION "."
)

install(FILES "$<TARGET_FILE:espeak-ng>"
    DESTINATION "."
)

install(DIRECTORY
    "${espeak-ng_SOURCE_DIR}/espeak-ng-data/"
    DESTINATION "espeak-ng-data"
    OPTIONAL
    FILES_MATCHING
    PATTERN "*"
    PATTERN ".git" EXCLUDE
    PATTERN "CMake*" EXCLUDE
)
