# Triangle Example
add_executable(triangle triangle/main.cpp)
target_link_libraries(triangle r267)

add_shader("triangle/main.vert" triangle)
add_shader("triangle/main.frag" triangle)

# Quad Example
add_executable(quad quad/main.cpp)
target_link_libraries(quad r267)

add_shader("quad/main.vert" quad)
add_shader("quad/main.frag" quad)

# Texture Example
add_executable(texture texture/main.cpp)
target_link_libraries(texture r267 OpenImageIO)

add_shader("texture/main.vert" texture)
add_shader("texture/main.frag" texture)

# Converter for Models
add_executable(converter converter/main.cpp)
target_link_libraries(converter r267 assimp)

# Scene loader example
add_executable(mesh mesh/main.cpp)
target_link_libraries(mesh r267)

add_shader("mesh/main.vert" mesh)
add_shader("mesh/main.frag" mesh)

# GUI example
add_executable(gui gui/main.cpp)
target_link_libraries(gui r267)

add_shader("gui/main.vert" gui)
add_shader("gui/main.frag" gui)
add_shader("gui/render.vert" gui)
add_shader("gui/render.frag" gui)

# Compute Shader example
add_executable(compute compute/main.cpp)
target_link_libraries(compute r267)

add_shader("compute/main.vert" compute)
add_shader("compute/main.frag" compute)
add_shader("compute/main.comp" compute)

# CubeMap example
add_executable(cubemap cubemap/main.cpp cubemap/ImageCube.cpp)
target_link_libraries(cubemap r267)

add_shader("cubemap/cubemap.vert" cubemap)
add_shader("cubemap/cubemap.frag" cubemap)
add_shader("cubemap/image2cube.vert" cubemap)
add_shader("cubemap/image2cube.frag" cubemap)
add_shader("cubemap/image2cube.geom" cubemap)
add_shader("cubemap/irradiance.frag" cubemap)
add_shader("cubemap/filter.frag" cubemap)
add_shader("cubemap/filter.vert" cubemap)
add_shader("cubemap/brdf.frag" cubemap)
add_shader("cubemap/brdf.vert" cubemap)