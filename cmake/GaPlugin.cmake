# GA plug-in build: JUCE plus everything every GA plug-in shares. After project():
#   include(framework/cmake/GaPlugin.cmake)
#   ga_add_plugin(MyPlugin PRODUCT_NAME "My Plugin" PLUGIN_CODE Abc1 BUNDLE_ID com.ga.myplugin SOURCES ${SOURCES})
include_guard(GLOBAL)

get_filename_component(GA_FRAMEWORK_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: next to the plug-in repo (../JUCE), otherwise -DJUCE_DIR=/path/to/JUCE. Every GA build uses
# the version in cmake/juce-version.txt, locally and in CI: get it with
#   git clone --depth 1 --branch <version> https://github.com/juce-framework/JUCE.git ../JUCE
file(STRINGS "${CMAKE_CURRENT_LIST_DIR}/juce-version.txt" GA_JUCE_VERSION LIMIT_COUNT 1)
get_filename_component(GA_JUCE_SIBLING "${CMAKE_SOURCE_DIR}/../JUCE" ABSOLUTE)
set(JUCE_DIR "" CACHE PATH "Path to the JUCE folder, if it is not next to the plug-in repo")
if(EXISTS "${GA_JUCE_SIBLING}/CMakeLists.txt")
    set(GA_JUCE_PATH "${GA_JUCE_SIBLING}")
elseif(EXISTS "${JUCE_DIR}/CMakeLists.txt")
    set(GA_JUCE_PATH "${JUCE_DIR}")
else()
    message(FATAL_ERROR "JUCE ${GA_JUCE_VERSION} not found at ${GA_JUCE_SIBLING}. Get it with:\n"
                        "  git clone --depth 1 --branch ${GA_JUCE_VERSION} https://github.com/juce-framework/JUCE.git ${GA_JUCE_SIBLING}\n"
                        "or point at another checkout with -DJUCE_DIR=/path/to/JUCE")
endif()
file(STRINGS "${GA_JUCE_PATH}/CMakeLists.txt" GA_JUCE_PROJECT REGEX "^project\\(JUCE VERSION ")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" GA_JUCE_FOUND "${GA_JUCE_PROJECT}")
if(NOT GA_JUCE_FOUND STREQUAL GA_JUCE_VERSION)
    message(FATAL_ERROR "JUCE at ${GA_JUCE_PATH} is ${GA_JUCE_FOUND}; GA plug-ins are built with ${GA_JUCE_VERSION} (cmake/juce-version.txt)")
endif()
add_subdirectory(${GA_JUCE_PATH} JUCE)

# ga_add_plugin(<target>
#     PRODUCT_NAME <name>          shown in hosts
#     PLUGIN_CODE <4 chars>        unique per plug-in; never change it after a release
#     BUNDLE_ID <id>
#     [COMPANY <name>]             default GA
#     [MANUFACTURER_CODE <4 chars>] default Gaau; never change it after a release
#     [NEEDS_MIDI_INPUT TRUE]      default FALSE
#     [AU_MAIN_TYPE <type>]        default kAudioUnitType_Effect; kAudioUnitType_MusicEffect if it needs MIDI
#     [VST3_CATEGORIES <...>]      default Fx
#     SOURCES <files...>)
#
# Shared by every GA plug-in: VST3 and AU, no copy after build
# (installing to /Library/Audio needs admin rights), link-time optimisation and dead-code stripping.
function(ga_add_plugin target)
    cmake_parse_arguments(GA "" "PRODUCT_NAME;PLUGIN_CODE;BUNDLE_ID;COMPANY;MANUFACTURER_CODE;NEEDS_MIDI_INPUT;AU_MAIN_TYPE" "VST3_CATEGORIES;SOURCES" ${ARGN})
    if(NOT GA_COMPANY)
        set(GA_COMPANY "GA")
    endif()
    if(NOT GA_MANUFACTURER_CODE)
        set(GA_MANUFACTURER_CODE Gaau)
    endif()
    if(NOT GA_NEEDS_MIDI_INPUT)
        set(GA_NEEDS_MIDI_INPUT FALSE)
    endif()
    if(NOT GA_AU_MAIN_TYPE)
        set(GA_AU_MAIN_TYPE kAudioUnitType_Effect)
    endif()
    if(NOT GA_VST3_CATEGORIES)
        set(GA_VST3_CATEGORIES Fx)
    endif()

    juce_add_plugin(${target}
        COMPANY_NAME "${GA_COMPANY}"
        BUNDLE_ID ${GA_BUNDLE_ID}
        PLUGIN_MANUFACTURER_CODE ${GA_MANUFACTURER_CODE}
        PLUGIN_CODE ${GA_PLUGIN_CODE}
        FORMATS VST3 AU
        PRODUCT_NAME "${GA_PRODUCT_NAME}"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT ${GA_NEEDS_MIDI_INPUT}
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_COPY_DIR "/Library/Audio/Plug-Ins/VST3"
        AU_COPY_DIR "/Library/Audio/Plug-Ins/Components"
        AU_MAIN_TYPE ${GA_AU_MAIN_TYPE}
        VST3_CATEGORIES ${GA_VST3_CATEGORIES})

    target_sources(${target} PRIVATE ${GA_SOURCES})

    # the framework's own code, included as e.g. #include "ui/Controls.h"
    file(GLOB GA_FRAMEWORK_SOURCES CONFIGURE_DEPENDS ${GA_FRAMEWORK_DIR}/ui/*.cpp ${GA_FRAMEWORK_DIR}/presets/*.cpp ${GA_FRAMEWORK_DIR}/dsp/*.cpp ${GA_FRAMEWORK_DIR}/plugin/*.cpp)
    target_sources(${target} PRIVATE ${GA_FRAMEWORK_SOURCES})
    target_include_directories(${target} PUBLIC ${GA_FRAMEWORK_DIR})

    target_compile_definitions(${target}
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0)

    target_link_libraries(${target}
        PRIVATE
            juce::juce_audio_processors
            juce::juce_gui_basics
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags)   # link-time optimisation: smaller and faster

    # leave out JUCE code the plug-in never calls
    target_link_options(${target} INTERFACE $<$<CONFIG:Release>:-Wl,-dead_strip>)
endfunction()

# ga_add_tool(<plugin> <tool> OPTION <option> [DESCRIPTION <text>] SOURCES <files...> MAIN <file>)
#
# A command-line tool (tests, screenshots) that runs the plug-in's own code. Built only when <option>
# is ON, so it never ends up in the plug-ins: SOURCES are compiled into the plug-in's shared code, with
# exactly the plug-in's JUCE configuration, and MAIN is a small entry point that calls into them.
function(ga_add_tool plugin tool)
    cmake_parse_arguments(T "" "OPTION;DESCRIPTION;MAIN" "SOURCES" ${ARGN})
    option(${T_OPTION} "${T_DESCRIPTION}" OFF)
    if(${T_OPTION})
        target_sources(${plugin} PRIVATE ${T_SOURCES})
        add_executable(${tool} ${T_MAIN})
        target_link_libraries(${tool} PRIVATE ${plugin})
    endif()
endfunction()
