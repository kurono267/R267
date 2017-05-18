# Triangle Example
add_executable(triangle triangle/main.cpp)
target_link_libraries(triangle ${GLFW_STATIC_LIBRARIES} ${Vulkan_LIBRARY} r267)

add_shader("triangle/main.vert" triangle)
add_shader("triangle/main.frag" triangle)

# Quad Example
add_executable(quad quad/main.cpp)
target_link_libraries(quad ${GLFW_STATIC_LIBRARIES} ${Vulkan_LIBRARY} r267)

add_shader("quad/main.vert" quad)
add_shader("quad/main.frag" quad)

# Texture Example
add_executable(texture texture/main.cpp)
target_link_libraries(texture ${GLFW_STATIC_LIBRARIES} ${Vulkan_LIBRARY} r267)

add_shader("texture/main.vert" texture)
add_shader("texture/main.frag" texture)
