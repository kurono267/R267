#include "MainApp.hpp"

using namespace r267;

GLFWKey::GLFWKey() : key(0), scancode(0), action(0), mods(0) {}

GLFWKey::GLFWKey(const GLFWKey& state) : key(state.key), scancode(state.scancode), action(state.action), mods(state.mods) {}
GLFWKey::GLFWKey(const int _key,const int _scancode,const int _action,const int _mods) : key(_key), scancode(_scancode), action(_action), mods(_mods) {}

GLFWKey::~GLFWKey(){}

GLFWMouse::GLFWMouse() : callState(noState), button(0),action(0),mods(0), x(0.0), y(0.0) {}

GLFWMouse::GLFWMouse(const GLFWMouse& state) : callState(state.callState), button(state.button), action(state.action), mods(state.mods), x(state.x), y(state.y) {}
GLFWMouse::GLFWMouse(const int _button,const int _action,const int _mods) : callState(onMouseButton), button(_button), action(_action), mods(_mods) {}
GLFWMouse::GLFWMouse(const double _x,const double _y) : callState(onMousePosition), x(_x), y(_y) {}

GLFWMouse::~GLFWMouse(){}

MainApp::MainApp() : _isRun(true) {
}

MainApp::~MainApp(){

}

void MainApp::setBaseApp(spBaseApp app){
	_app = app;
}

void MainApp::exit(){
	_isRun = false;
}

bool MainApp::is(){
	return _isRun;
}

spInstance MainApp::vulkan(){
	return _vulkan;
}

GLFWwindow* MainApp::window(){
	return _window;
}

void MainApp::resize(const int width,const int height){
	// Relase
	_vulkan->device()->getDevice().waitIdle();
	_app->onExit();
	_vulkan->release();
	// Reinit with new resolution
	_vulkan = std::make_shared<Instance>();
	_vulkan->init(_title,_window,glm::ivec2(width,height));
	_app->init();
}
			
void MainApp::run(){
	_app->mainApp = shared_from_this();
	_app->init();
	while(_isRun){
		//glFinish();
		_app->update();
		_app->draw();
		_vulkan->device()->getDevice().waitIdle();
		glfwPollEvents();

		if(glfwWindowShouldClose(_window))_isRun = false;
	}
	_vulkan->device()->getDevice().waitIdle();
	_app->onExit();
	_vulkan->release();
	glfwDestroyWindow(_window);
	glfwTerminate();
	::exit(EXIT_SUCCESS);
}

glm::ivec2 MainApp::wndSize(){
	int width;
	int height;
	glfwGetWindowSize(_window, &width, &height);
	return glm::vec2(width,height);
}

void MainApp::create(const std::string& title,const int width,const int height){
	try {
		glfwSetErrorCallback(&MainApp::__glfwOnError);
		if (!glfwInit())
			::exit(EXIT_FAILURE);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		if (!_window){
			glfwTerminate();
			::exit(EXIT_FAILURE);
		}
		glfwSetKeyCallback(_window, &MainApp::__glfwOnKey);
		glfwSetCursorPosCallback(_window, &MainApp::__glfwOnMousePos);
		glfwSetMouseButtonCallback(_window, &MainApp::__glfwOnMouseBtn);
		glfwSetScrollCallback(_window, &MainApp::__glfwOnScroll);
		glfwSetWindowSizeCallback(_window, &MainApp::__glfwOnResize);

		_vulkan = std::make_shared<Instance>();
		_title = title;
		_vulkan->init(title,_window,wndSize());
	} catch(std::exception& e){
		std::cout << e.what() << std::endl;
	}
}

void MainApp::vsync(bool on){
	if(on)glfwSwapInterval(1);
	else glfwSwapInterval(0);
}

const GLFWKey MainApp::glfwKeyState() const {
	return _lastKey;
}

const GLFWMouse MainApp::glfwMouseState() const {
	return _lastMouse;
}
