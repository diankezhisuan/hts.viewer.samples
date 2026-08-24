add_executable(viewer_interaction main.cpp)
target_include_directories(viewer_interaction PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../common")
configure_hts_viewer_sample(viewer_interaction)
