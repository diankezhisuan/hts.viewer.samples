add_executable(viewer_appearance main.cpp)
target_include_directories(viewer_appearance PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../common")
configure_hts_viewer_sample(viewer_appearance)
