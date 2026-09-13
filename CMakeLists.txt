cmake_minimum_required(VERSION 3.20)
project(DoodleFight LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COCOS2D_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cocos2d" CACHE PATH "Path to Cocos2d-x")

if(NOT EXISTS "${COCOS2D_ROOT_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Cocos2d-x was not found. Place/clone Cocos2d-x in ./cocos2d or set COCOS2D_ROOT_PATH.")
endif()

add_subdirectory(${COCOS2D_ROOT_PATH} cocos2d-build)

set(GAME_SOURCES
    Classes/AppDelegate.cpp
    Classes/GameScene.cpp
    Classes/Player.cpp
    Classes/Combat/Projectile.cpp
    Classes/Combat/Weapon.cpp
    Classes/Network/NetworkManager.cpp
)

set(GAME_HEADERS
    Classes/AppDelegate.h
    Classes/GameScene.h
    Classes/Player.h
    Classes/Combat/Projectile.h
    Classes/Combat/Weapon.h
    Classes/Network/NetworkManager.h
)

add_library(doodle_fight_game STATIC ${GAME_SOURCES} ${GAME_HEADERS})
target_include_directories(doodle_fight_game PUBLIC Classes)
target_link_libraries(doodle_fight_game PUBLIC cocos2d)
