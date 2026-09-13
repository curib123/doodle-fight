cmake_minimum_required(VERSION 3.20)

set(APP_NAME doodle_fight)
project(${APP_NAME} LANGUAGES C CXX)

set(COCOS2DX_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cocos2d" CACHE PATH "Path to Cocos2d-x")

if(NOT EXISTS "${COCOS2DX_ROOT_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "Cocos2d-x was not found at ${COCOS2DX_ROOT_PATH}. Run scripts/setup_cocos.ps1 first or set COCOS2DX_ROOT_PATH.")
endif()

# Follow the Cocos2d-x v4 application-template integration instead of adding
# the engine repository root (which also builds engine tests/templates).
set(CMAKE_MODULE_PATH "${COCOS2DX_ROOT_PATH}/cmake/Modules/")
include(CocosBuildSet)
add_subdirectory("${COCOS2DX_ROOT_PATH}/cocos" "${ENGINE_BINARY_PATH}/cocos/core")

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
    Classes/Network/Protocol.h
)

if(WINDOWS)
    list(APPEND GAME_SOURCES proj.win32/main.cpp)
endif()

add_executable(${APP_NAME} WIN32 ${GAME_SOURCES} ${GAME_HEADERS})

target_compile_features(${APP_NAME} PRIVATE cxx_std_17)
target_include_directories(${APP_NAME}
    PRIVATE
        Classes
        "${COCOS2DX_ROOT_PATH}/cocos/audio/include"
)

target_link_libraries(${APP_NAME} PRIVATE cocos2d)

if(WINDOWS)
    target_link_libraries(${APP_NAME} PRIVATE ws2_32)
endif()

# Apply the normal Cocos app settings and copy runtime DLLs next to the EXE.
setup_cocos_app_config(${APP_NAME})

if(WINDOWS)
    cocos_copy_target_dll(${APP_NAME})
endif()

# Resources are optional while the prototype still uses generated/vector art.
set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/Resources")
if(EXISTS "${GAME_RES_FOLDER}" AND (WINDOWS OR LINUX))
    cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
    cocos_copy_target_res(${APP_NAME} LINK_TO ${APP_RES_DIR} FOLDERS ${GAME_RES_FOLDER})
endif()
