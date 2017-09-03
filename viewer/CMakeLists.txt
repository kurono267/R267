# Models viewer
add_executable(viewer src/main.cpp src/viewer.cpp src/toolbar.cpp src/gbuffer.cpp src/ssao.cpp)
target_link_libraries(viewer r267)

# Simple material shaders
add_shader("effects/differed.vert" viewer)
add_shader("effects/differed.frag" viewer)
add_shader("effects/gbuffer.vert" viewer)
add_shader("effects/gbuffer.frag" viewer)
add_shader("effects/ssao.vert" viewer)
add_shader("effects/ssao.frag" viewer)