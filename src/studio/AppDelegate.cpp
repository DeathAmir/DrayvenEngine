#include "AppDelegate.hpp"
#include "StudioScene.hpp"
#include "FairyGUI.h"

USING_NS_CC;
USING_NS_FGUI;

void AppDelegate::initGLContextAttrs() {
    GLContextAttrs attrs{8, 8, 8, 8, 24, 8};
    GLView::setGLContextAttrs(attrs);
}

bool AppDelegate::applicationDidFinishLaunching() {
    auto* director = Director::getInstance();
    auto* view = director->getOpenGLView();
    if (!view) {
        view = GLViewImpl::createWithRect("Drayven Studio", Rect(80, 60, 1440, 900));
        director->setOpenGLView(view);
    }

    director->setDisplayStats(false);
    director->setAnimationInterval(1.0f / 60.0f);
    director->setClearColor(Color4F(Color4B(24, 27, 32, 255)));
    view->setDesignResolutionSize(1440.0f, 900.0f, ResolutionPolicy::SHOW_ALL);

    auto* scene = StudioScene::create();
    director->runWithScene(scene);
    return true;
}

void AppDelegate::applicationDidEnterBackground() {
    Director::getInstance()->stopAnimation();
}

void AppDelegate::applicationWillEnterForeground() {
    Director::getInstance()->startAnimation();
}
