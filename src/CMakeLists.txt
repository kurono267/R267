set ( R267_SRC 
base/wnd/MainApp.cpp 
base/vk/instance.cpp 
base/vk/device.cpp 
base/vk/swapchain.cpp 
base/vk/utils.cpp 
base/vk/pipeline.cpp 
base/vk/buffer.cpp 
base/vk/uniform.cpp 
base/vk/additional.cpp 
base/vk/shape.cpp 
base/vk/image.cpp
base/vk/framebuffer.cpp

base/scene/mesh.cpp
base/scene/material.cpp
base/scene/model.cpp
base/scene/scene.cpp
base/scene/camera.cpp

base/gui/gui.cpp
)

add_library(r267 SHARED ${R267_SRC})
target_link_libraries(r267 ${GLFW_STATIC_LIBRARIES} ${Vulkan_LIBRARY} OpenImageIO stdc++fs)