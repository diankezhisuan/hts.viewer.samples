add_executable(viewer_mesh main.cpp)
target_include_directories(viewer_mesh PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../common")
configure_hts_viewer_sample(viewer_mesh)
