# Models viewer
add_executable(viewer src/main.cpp src/viewer.cpp src/toolbar.cpp)
target_link_libraries(viewer r267)

# Simple material shaders
add_shader("viewer/basic.vert" viewer)
add_shader("viewer/basic.frag" viewer)