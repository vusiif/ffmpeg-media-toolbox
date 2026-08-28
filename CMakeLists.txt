cmake_minimum_required(VERSION 3.21)

project(ffmedia-toolbox
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED CMAKE_PREFIX_PATH)
    if(DEFINED ENV{Qt6_DIR})
        set(CMAKE_PREFIX_PATH "$ENV{Qt6_DIR}")
    elseif(DEFINED ENV{QT_DIR})
        set(CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
    elseif(EXISTS "C:/Qt/6.11.1/msvc2022_64")
        set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
    elseif(EXISTS "$ENV{HOME}/Qt/6.11.1/msvc2022_64")
        set(CMAKE_PREFIX_PATH "$ENV{HOME}/Qt/6.11.1/msvc2022_64")
    endif()
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    Qml
    QuickControls2
)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

set(APP_SOURCES
    src/main.cpp
    src/ffmpeg/FFmpegLocator.h
    src/ffmpeg/FFmpegLocator.cpp
)

set(QML_SOURCES
    qml/Main.qml
)

qt_add_executable(ffmedia-toolbox
    ${APP_SOURCES}
)

qt_add_qml_module(ffmedia-toolbox
    URI ffmedia
    VERSION 1.0
    QML_FILES
        ${QML_SOURCES}
)

target_include_directories(ffmedia-toolbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ffmedia-toolbox PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
)

qt_generate_deploy_app_script(
    TARGET ffmedia-toolbox
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
)
install(SCRIPT ${deploy_script})

add_custom_command(TARGET ffmedia-toolbox POST_BUILD
    COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe"
        --qmldir "${CMAKE_SOURCE_DIR}/qml"
        --no-translations
        --no-opengl-sw
        --no-compiler-runtime
        "$<TARGET_FILE:ffmedia-toolbox>"
    COMMENT "Deploying Qt runtime..."
)
