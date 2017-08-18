//
// Created by kurono267 on 05.08.17.
//

#pragma once

#include <chrono>

#include <base/wnd/MainApp.hpp>
#include <base/vk/pipeline.hpp>
#include <base/vk/additional.hpp>
#include <base/vk/shape.hpp>

#include "toolbar.hpp"
#include <base/gui/gui.hpp>

#include <base/scene/scene.hpp>
#include <base/scene/camera.hpp>

using namespace r267;

struct UBO {
    glm::mat4 mvp;
    glm::vec4 view;
};

class ViewerApp : public BaseApp {
public:
    ViewerApp(spMainApp app,const std::string& filename) : BaseApp(app),_filename(filename),_isPressed(false),_isFirst(false) {}
    virtual ~ViewerApp(){}

    bool init();
    bool draw();
    bool update();

    bool onKey(const GLFWKey& key){}

    bool onMouse(const GLFWMouse& mouse){
        if(_guiEvents)return true;
        bool status = false;
        if(mouse.callState == GLFWMouse::onMouseButton){
            if(mouse.button == GLFW_MOUSE_BUTTON_1){
                if(mouse.action == GLFW_PRESS){
                    _isPressed = true;
                    _isFirst = true;
                } else _isPressed = false;
            }
        } else if (mouse.callState == GLFWMouse::onMousePosition){
            if(_isPressed){
                if(!_isFirst){
                    glm::vec2 dp = glm::vec2(mouse.x,mouse.y)-_prev_mouse;
                    _camera->rotate(dp,_dt);
                    _mvpData.view = glm::vec4(_camera->getPos(),1.0f);
                    _mvpData.mvp = _camera->getVP();
                    status = true;
                }
                _prev_mouse = glm::vec2(mouse.x,mouse.y);
                _isFirst = false;
            }
        }
        return status;
    }
    bool onScroll(const glm::vec2& offset){
        if(_guiEvents)return true;
        _camera->scale(offset.y,_dt);
        _mvpData.view = glm::vec4(_camera->getPos(),1.0f);
        _mvpData.mvp = _camera->getVP();
        return true;
    }

    bool onExit(){
        vulkan = mainApp->vulkan();device = vulkan->device();vk_device = device->getDevice();
        vk_device.freeCommandBuffers(device->getCommandPool(),_commandBuffers);
        _main->release();
        return true;
    }
protected:
    std::string _filename;

    spDescSet  _sceneDesc;
    UBO        _mvpData;
    Uniform    _mvp;

    spPipeline _main;
    spGUI      _gui;
    bool       _guiEvents;

    updateGUI  _guiFunc;
    Toolbar    _toolbar;

    spScene  _scene;
    spCamera _camera;

    // Framebuffers
    std::vector<spFramebuffer> _framebuffers;
    // Command Buffers
    vk::CommandPool _commandPool;
    std::vector<vk::CommandBuffer> _commandBuffers;
    // Semaphores
    vk::Semaphore _imageAvailable;
    vk::Semaphore _renderFinish;

    std::unordered_map<std::string,spImage> _imagesBuffer;

    // For camera rotate
    bool _isPressed;
    bool _isFirst;
    glm::vec2 _prev_mouse;
    float _dt;

    std::chrono::time_point<std::chrono::steady_clock> prev;
};
