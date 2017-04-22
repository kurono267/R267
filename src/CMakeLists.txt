set(R267_SRC base/wnd/MainApp.cpp base/vk/instance.cpp base/vk/device.cpp base/vk/swapchain.cpp base/vk/utils.cpp base/vk/pipeline.cpp base/vk/buffer.cpp base/vk/uniform.cpp)

add_library(r267 SHARED ${R267_SRC} ${Vulkan_LIBRARY})