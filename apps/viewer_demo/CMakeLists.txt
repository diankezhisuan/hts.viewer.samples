cmake_minimum_required(VERSION 3.18)

# Support both of the documented entry points:
#   cmake -S . -B build
#   cmake -S apps/viewer_demo -B build
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(HtsViewerDemo VERSION 2.0 LANGUAGES CXX)
    if(NOT WIN32)
        message(FATAL_ERROR "The current SDK delivery contains Windows binaries only.")
    endif()
    if(CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING
            "Supported build configurations" FORCE)
    elseif(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    endif()
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(NOT TARGET Hts3d::ViewerSDK)
    set(HTS_VIEWER_SDK_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/../../ref/hts.viewer/win"
        CACHE PATH "Hts Viewer SDK delivery root")
    set(HTS_VIEWER_INCLUDE_DIR "${HTS_VIEWER_SDK_ROOT}/include")
    set(HTS_VIEWER_IMPLIB "${HTS_VIEWER_SDK_ROOT}/lib/Hts3dViewerSDK.lib")
    set(HTS_VIEWER_RUNTIME "${HTS_VIEWER_SDK_ROOT}/bin/Hts3dViewerSDK.dll")
    set(HTS_VIEWER_RUNTIME_DIR "${HTS_VIEWER_SDK_ROOT}/bin")
    foreach(REQUIRED_SDK_FILE
            "${HTS_VIEWER_INCLUDE_DIR}/HtsViewerSdk.h"
            "${HTS_VIEWER_IMPLIB}"
            "${HTS_VIEWER_RUNTIME}"
            "${HTS_VIEWER_RUNTIME_DIR}/hts_auth.dll"
            "${HTS_VIEWER_RUNTIME_DIR}/libcrypto-3-x64.dll"
            "${HTS_VIEWER_RUNTIME_DIR}/libssl-3-x64.dll"
            "${HTS_VIEWER_RUNTIME_DIR}/legacy.dll"
            "${HTS_VIEWER_RUNTIME_DIR}/libcurl.dll"
            "${HTS_VIEWER_RUNTIME_DIR}/zlib.dll")
        if(NOT EXISTS "${REQUIRED_SDK_FILE}")
            message(FATAL_ERROR
                    "Hts Viewer SDK delivery is incomplete: ${REQUIRED_SDK_FILE}")
        endif()
    endforeach()
    add_library(Hts3dViewerSDK SHARED IMPORTED GLOBAL)
    add_library(Hts3d::ViewerSDK ALIAS Hts3dViewerSDK)
    set_target_properties(Hts3dViewerSDK PROPERTIES
        IMPORTED_IMPLIB "${HTS_VIEWER_IMPLIB}"
        IMPORTED_LOCATION "${HTS_VIEWER_RUNTIME}"
        INTERFACE_INCLUDE_DIRECTORIES "${HTS_VIEWER_INCLUDE_DIR}")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(QT_ROOT "" CACHE PATH "Optional Qt 5 installation root")
if(QT_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${QT_ROOT}")
endif()
find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets OpenGL)

option(HTS_ENABLE_OCCT_IMPORT "Enable STEP/IGES import" ON)
set(OCCT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/private/occt/win"
    CACHE PATH "Viewer Demo private OpenCASCADE delivery root")

if(HTS_ENABLE_OCCT_IMPORT)
    if(IS_DIRECTORY "${OCCT_ROOT}/include/opencascade")
        set(OCCT_INCLUDE_DIR "${OCCT_ROOT}/include/opencascade")
    else()
        set(OCCT_INCLUDE_DIR "${OCCT_ROOT}/include")
    endif()
    set(OCCT_LIB_DIR "${OCCT_ROOT}/lib")
    set(OCCT_BIN_DIR "${OCCT_ROOT}/bin")

    foreach(REQUIRED_OCCT_DIR
            "${OCCT_INCLUDE_DIR}" "${OCCT_LIB_DIR}" "${OCCT_BIN_DIR}")
        if(NOT IS_DIRECTORY "${REQUIRED_OCCT_DIR}")
            message(FATAL_ERROR "OCCT delivery is incomplete: ${REQUIRED_OCCT_DIR}")
        endif()
    endforeach()

    set(HTS_OCCT_LIB_NAMES
        TKernel TKMath TKG2d TKG3d TKGeomBase TKGeomAlgo TKBRep TKTopAlgo
        TKPrim TKBO TKShHealing TKMesh TKXSBase TKDE TKDESTEP TKDEIGES
        TKXCAF TKCAF TKLCAF TKVCAF)
    set(HTS_OCCT_LIBS)
    foreach(OCCT_LIB_NAME IN LISTS HTS_OCCT_LIB_NAMES)
        set(OCCT_LIBRARY "${OCCT_LIB_DIR}/${OCCT_LIB_NAME}.lib")
        if(NOT EXISTS "${OCCT_LIBRARY}")
            message(FATAL_ERROR "Required OCCT library is missing: ${OCCT_LIBRARY}")
        endif()
        list(APPEND HTS_OCCT_LIBS "${OCCT_LIBRARY}")
    endforeach()

    # Runtime closure measured from HtsViewerDemoImporter.dll and resolved
    # transitively against this OCCT delivery. Keep OCCT deployment private to
    # the product Demo; the Viewer SDK and ordinary samples never consume it.
    set(HTS_OCCT_RUNTIME_NAMES
        TKBO.dll TKBool.dll TKBRep.dll TKCAF.dll TKCDF.dll TKDE.dll
        TKDEIGES.dll TKDESTEP.dll TKernel.dll TKG2d.dll TKG3d.dll
        TKGeomAlgo.dll TKGeomBase.dll TKHLR.dll TKLCAF.dll TKMath.dll
        TKMesh.dll TKPrim.dll TKService.dll TKShHealing.dll TKTopAlgo.dll
        TKV3d.dll TKVCAF.dll TKXCAF.dll TKXSBase.dll

        # Transitive runtime dependencies of this OCCT delivery. They remain
        # private to HtsViewerDemoImporter and must never enter ref/hts.viewer.
        # Note: this TKernel imports the name tbb12_debug.dll even in its
        # Release binary. The private package intentionally stores the matching
        # Release tbb12.dll binary under that required name, so no Debug CRT is
        # introduced into the distributable application.
        avcodec-57.dll avformat-57.dll avutil-55.dll swscale-4.dll
        FreeImage.dll freetype.dll jemalloc.dll openvr_api.dll
        tbb12_debug.dll)
    set(HTS_OCCT_RUNTIME_DLLS)
    foreach(OCCT_RUNTIME_NAME IN LISTS HTS_OCCT_RUNTIME_NAMES)
        set(OCCT_RUNTIME_DLL "${OCCT_BIN_DIR}/${OCCT_RUNTIME_NAME}")
        if(NOT EXISTS "${OCCT_RUNTIME_DLL}")
            message(FATAL_ERROR "Required OCCT runtime is missing: ${OCCT_RUNTIME_DLL}")
        endif()
        list(APPEND HTS_OCCT_RUNTIME_DLLS "${OCCT_RUNTIME_DLL}")
    endforeach()
endif()

add_library(HtsViewerDemoImporter SHARED
    import/HtsViewerDemoImporterApi.h
    import/HtsImportOptions.h
    import/HtsImportedModel.h
    import/HtsDisplayDataImporter.h
    import/HtsDisplayDataImporter.cpp
    import/HtsImportPathCodec.h
    import/HtsImportPathCodec.cpp
    import/HtsOccImportAdapter.h
    import/HtsOccImportAdapter.cpp
    import/HtsImportMemoryGuard.h
    import/HtsImportMemoryGuard.cpp
    import/HtsOccShapeDisplayDataBuilder.h
    import/HtsOccShapeDisplayDataBuilder.cpp)

target_compile_definitions(HtsViewerDemoImporter PRIVATE
    HTS_VIEWER_DEMO_IMPORTER_BUILD)
target_include_directories(HtsViewerDemoImporter
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${HTS_VIEWER_INCLUDE_DIR}")
target_link_libraries(HtsViewerDemoImporter PRIVATE Qt5::Core Psapi)
set_target_properties(HtsViewerDemoImporter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

if(MSVC)
    target_compile_options(HtsViewerDemoImporter PRIVATE /utf-8)
endif()

if(HTS_ENABLE_OCCT_IMPORT)
    target_compile_definitions(HtsViewerDemoImporter PRIVATE
        HTS_ENABLE_OCCT_IMPORT
        HTS_OCCT_USE_STEP_CONTROL_READER
        HTS_OCCT_USE_STEP_CAF_READER
        HTS_OCCT_USE_IGES_CONTROL_READER)
    target_include_directories(HtsViewerDemoImporter SYSTEM PRIVATE "${OCCT_INCLUDE_DIR}")
    target_link_libraries(HtsViewerDemoImporter PRIVATE ${HTS_OCCT_LIBS})
endif()

add_executable(viewer_demo WIN32
    main.cpp
    ViewerMainWindow.h
    ViewerMainWindow.cpp
    LicenseInfoDialog.h
    LicenseInfoDialog.cpp
    LicenseInfoDialog.ui
    viewer/widget/HtsViewerWidget.h
    viewer/widget/HtsViewerWidget.cpp)

target_include_directories(viewer_demo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(viewer_demo PRIVATE
    Qt5::Widgets
    Qt5::OpenGL
    Hts3d::ViewerSDK
    HtsViewerDemoImporter)

if(MSVC)
    target_compile_options(viewer_demo PRIVATE /utf-8)
endif()

set_target_properties(viewer_demo PROPERTIES
    OUTPUT_NAME "HtsViewerDemo"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:viewer_demo>")
set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
             PROPERTY VS_STARTUP_PROJECT viewer_demo)

install(TARGETS viewer_demo HtsViewerDemoImporter
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib)

add_custom_command(TARGET viewer_demo POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${HTS_VIEWER_RUNTIME_DIR}"
            "$<TARGET_FILE_DIR:viewer_demo>"
    COMMENT "Deploy Hts Viewer SDK and authorization runtime")

if(HTS_ENABLE_OCCT_IMPORT)
    foreach(OCCT_RUNTIME_DLL IN LISTS HTS_OCCT_RUNTIME_DLLS)
        add_custom_command(TARGET viewer_demo POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    "${OCCT_RUNTIME_DLL}"
                    "$<TARGET_FILE_DIR:viewer_demo>"
            COMMENT "Deploy ${OCCT_RUNTIME_DLL}")
    endforeach()
    install(FILES ${HTS_OCCT_RUNTIME_DLLS} DESTINATION bin)
endif()

if(TARGET Qt5::qmake)
    get_target_property(QT_QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET viewer_demo POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --no-translations
                    --no-system-d3d-compiler
                    "$<TARGET_FILE:viewer_demo>"
            COMMENT "Deploy Qt runtime")
        install(CODE
            "execute_process(COMMAND \"${WINDEPLOYQT_EXECUTABLE}\" --no-translations --no-system-d3d-compiler \"\${CMAKE_INSTALL_PREFIX}/bin/HtsViewerDemo.exe\" RESULT_VARIABLE HTS_WINDEPLOYQT_RESULT)\nif(NOT HTS_WINDEPLOYQT_RESULT EQUAL 0)\n  message(FATAL_ERROR \"windeployqt failed during install: \${HTS_WINDEPLOYQT_RESULT}\")\nendif()")
    else()
        message(WARNING "windeployqt was not found; deploy Qt runtime manually.")
    endif()
endif()
