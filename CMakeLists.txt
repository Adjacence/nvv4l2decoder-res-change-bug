cmake_minimum_required(VERSION 3.0)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED gstreamer-1.0)

add_executable(nvv4l2decoder-res-change-bug main.c)
target_compile_options(nvv4l2decoder-res-change-bug PUBLIC ${GST_CFLAGS})
target_include_directories(nvv4l2decoder-res-change-bug
                           PUBLIC ${GST_INCLUDE_DIRS})
target_link_libraries(nvv4l2decoder-res-change-bug ${GST_LIBRARIES})
