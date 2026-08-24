cmake_minimum_required(VERSION 3.18)

project(HtsViewerSamples VERSION 2.0 LANGUAGES CXX)

include(GNUInstallDirs)

if(NOT WIN32)
    message(FATAL_ERROR "The current SDK delivery contains Windows binaries only.")
endif()

#---------------------------------------
# Visual Studio Solution organization
#---------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

# Hts3dViewerSDK is delivered as a Release binary and its public ABI contains
# STL types. Keep consumers on the same MSVC runtime/configuration.
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
        "Supported build configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(HTS_VIEWER_SDK_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/ref/hts.viewer/win"
    CACHE PATH "Hts Viewer SDK delivery root")
set(HTS_VIEWER_INCLUDE_DIR "${HTS_VIEWER_SDK_ROOT}/include")
set(HTS_VIEWER_IMPLIB "${HTS_VIEWER_SDK_ROOT}/lib/Hts3dViewerSDK.lib")
set(HTS_VIEWER_RUNTIME "${HTS_VIEWER_SDK_ROOT}/bin/Hts3dViewerSDK.dll")
set(HTS_VIEWER_RUNTIME_DIR "${HTS_VIEWER_SDK_ROOT}/bin")

foreach(REQUIRED_SDK_FILE
        "${HTS_VIEWER_INCLUDE_DIR}/HtsViewerSdk.h"
        "${HTS_VIEWER_INCLUDE_DIR}/HtsViewerTypes.h"
        "${HTS_VIEWER_INCLUDE_DIR}/HtsViewerSettings.h"
        "${HTS_VIEWER_INCLUDE_DIR}/HtsMeshData.h"
        "${HTS_VIEWER_INCLUDE_DIR}/HtsDisplayData.h"
        "${HTS_VIEWER_IMPLIB}"
        "${HTS_VIEWER_RUNTIME}"
        "${HTS_VIEWER_RUNTIME_DIR}/hts_auth.dll"
        "${HTS_VIEWER_RUNTIME_DIR}/libcrypto-3-x64.dll"
        "${HTS_VIEWER_RUNTIME_DIR}/libssl-3-x64.dll"
        "${HTS_VIEWER_RUNTIME_DIR}/legacy.dll"
        "${HTS_VIEWER_RUNTIME_DIR}/libcurl.dll"
        "${HTS_VIEWER_RUNTIME_DIR}/zlib.dll")
    if(NOT EXISTS "${REQUIRED_SDK_FILE}")
        message(FATAL_ERROR "Hts Viewer SDK delivery is incomplete: ${REQUIRED_SDK_FILE}")
    endif()
endforeach()

add_library(Hts3dViewerSDK SHARED IMPORTED GLOBAL)
add_library(Hts3d::ViewerSDK ALIAS Hts3dViewerSDK)

set_target_properties(Hts3dViewerSDK PROPERTIES
    IMPORTED_IMPLIB "${HTS_VIEWER_IMPLIB}"
    IMPORTED_LOCATION "${HTS_VIEWER_RUNTIME}"
    INTERFACE_INCLUDE_DIRECTORIES "${HTS_VIEWER_INCLUDE_DIR}")

option(HTS_VIEWER_BUILD_DEMO "Build the Qt Viewer product demo" ON)
option(HTS_VIEWER_BUILD_API_SAMPLES "Build the small SDK API samples" ON)

# Qt is only required by viewer_demo. The installation location is local to
# each developer machine and can be supplied with -DQT_ROOT=<path>.
set(QT_ROOT "" CACHE PATH "Optional Qt 5 installation root used by viewer_demo")

#---------------------------------------
# Product Demo
# viewer_demo intentionally stays at the Solution root.
#---------------------------------------
if(HTS_VIEWER_BUILD_DEMO)
    add_subdirectory(apps/viewer_demo)

    # Demo private implementation should not compete with the public demo
    # for attention in Visual Studio Solution Explorer.
    if(TARGET HtsViewerDemoImporter)
        set_target_properties(HtsViewerDemoImporter PROPERTIES
            FOLDER "_Internal")
    endif()

    # Make the complete Viewer product demo the default Visual Studio
    # startup project.
    if(TARGET viewer_demo)
        set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            PROPERTY VS_STARTUP_PROJECT viewer_demo)
    endif()
endif()

#---------------------------------------
# API Samples
#---------------------------------------
if(HTS_VIEWER_BUILD_API_SAMPLES)
    set(PLATFORM_FOLDER win)
    set(HTS_VIEWER_SAMPLE_OUTPUT_ROOT "${CMAKE_BINARY_DIR}/samples/bin")

    function(configure_hts_viewer_sample TARGET_NAME)
        target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
        target_link_libraries(${TARGET_NAME} PRIVATE Hts3d::ViewerSDK)

        if(MSVC)
            target_compile_options(${TARGET_NAME} PRIVATE /utf-8)
        endif()

        set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${HTS_VIEWER_SAMPLE_OUTPUT_ROOT}/$<CONFIG>")

        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${HTS_VIEWER_RUNTIME_DIR}"
                    "$<TARGET_FILE_DIR:${TARGET_NAME}>"
            COMMENT "Deploy Viewer SDK runtime for ${TARGET_NAME}")

        install(TARGETS ${TARGET_NAME}
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endfunction()

    add_subdirectory(samples)

    # Keep all API teaching samples together in Visual Studio.
    set(HTS_VIEWER_SAMPLE_TARGETS
        viewer_license
        viewer_quickstart
        viewer_interaction
        viewer_appearance
        viewer_mesh
    )

    foreach(SAMPLE_TARGET IN LISTS HTS_VIEWER_SAMPLE_TARGETS)
        if(TARGET ${SAMPLE_TARGET})
            set_target_properties(${SAMPLE_TARGET} PROPERTIES
                FOLDER "Samples")
        endif()
    endforeach()
endif()

# Install one shared Viewer runtime closure for Demo and API samples.
install(DIRECTORY "${HTS_VIEWER_RUNTIME_DIR}/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
    FILES_MATCHING PATTERN "*.dll")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    DESTINATION ".")
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Documentation")
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Documentation/"
        DESTINATION "Documentation")
endif()

set(CPACK_PACKAGE_NAME "HtsViewerSamples")
set(CPACK_PACKAGE_VENDOR "Chengdu Dianke Zhishuan Technology Co., Ltd.")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "HtsViewerSamples-${PROJECT_VERSION}-win-x64")
set(CPACK_GENERATOR "ZIP")
include(CPack)

message(STATUS "Hts Viewer SDK root: ${HTS_VIEWER_SDK_ROOT}")
