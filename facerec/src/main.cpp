#include "ofMain.h"
#include "ofApp.h"

int main(int argc, char **argv)
{
    ofGLWindowSettings settings;
    settings.setSize(1024, 768);
    settings.title = "facerec";

    auto window = ofCreateWindow(settings);
    auto app = std::make_shared<ofApp>();
    for (int i = 1; i < argc; i++)
    {
        app->args.push_back(argv[i]);
    }
    ofRunApp(window, app);
    return ofRunMainLoop();
}
