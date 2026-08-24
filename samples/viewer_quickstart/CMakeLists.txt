add_executable(viewer_quickstart main.cpp)
target_include_directories(viewer_quickstart PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../common")
configure_hts_viewer_sample(viewer_quickstart)
