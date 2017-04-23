add_executable(triangle triangle/main.cpp)
target_link_libraries(triangle ${GLFW_STATIC_LIBRARIES} ${Vulkan_LIBRARY} r267)

add_shader("triangle/main.vert" triangle)
add_shader("triangle/main.frag" triangle)
