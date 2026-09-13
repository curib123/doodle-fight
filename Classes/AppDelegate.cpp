#include "AppDelegate.h"
#include "GameScene.h"

USING_NS_CC;

void AppDelegate::initGLContextAttrs() {
    GLContextAttrs attrs{8, 8, 8, 8, 24, 8};
    GLView::setGLContextAttrs(attrs);
}

bool AppDelegate::applicationDidFinishLaunching() {
    auto* director = Director::getInstance();
    auto* glview = director->getOpenGLView();

    if (!glview) {
        glview = GLViewImpl::create("Doodle Fight");
        director->setOpenGLView(glview);
    }

    director->setAnimationInterval(1.0f / 60.0f);
    director->runWithScene(GameScene::create());
    return true;
}

void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();
}

void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();
}
