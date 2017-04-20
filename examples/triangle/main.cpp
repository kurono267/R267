#include "base/wnd/MainApp.hpp"

using namespace r267;

class TriangleApp : public BaseApp {
	public:
		TriangleApp(spMainApp app) : BaseApp(app) {}
		virtual ~TriangleApp(){}

		bool init(){
			return true;
		}
		bool draw(){
			return true;
		}
		bool update(){
			return true;
		}
		
		bool onKey(const GLFWKey& key){}
		bool onMouse(const GLFWMouse& mouse){}
		bool onExit(){}
	protected:
};

int main(){
	spMainApp main = MainApp::instance();
	spBaseApp app = std::make_shared<TriangleApp>(main);

	main->create("Triangle",1280,720);
	main->setBaseApp(app);

	main->run();

	return 0;
}