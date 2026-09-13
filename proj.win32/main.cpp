#include "AppDelegate.h"
#include "cocos2d.h"

#if defined(_WIN32)
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    AppDelegate app;
    return cocos2d::Application::getInstance()->run();
}
#endif
