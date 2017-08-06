#include "viewer.hpp"

using namespace r267;

int main(int argc, char* argv[]){
    if(argc < 2){
        std::cout << "Scene filename doesn't set" << std::endl;
        return -1;
    }
	spMainApp main = MainApp::instance();
	spBaseApp app = std::make_shared<ViewerApp>(main,argv[1]);

	main->create("Viewer",1280,720);
	main->setBaseApp(app);

	main->run();

	return 0;
}