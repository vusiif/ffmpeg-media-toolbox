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

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

if(NOT DEFINED CMAKE_PREFIX_PATH)
    if(DEFINED ENV{Qt6_DIR})
        set(CMAKE_PREFIX_PATH "$ENV{Qt6_DIR}")
    elseif(DEFINED ENV{QT_DIR})
        set(CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
    elseif(EXISTS "C:/Qt/6.11.1/msvc2022_64")
        set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
    endif()
endif()

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS
    Concurrent
    Widgets
    WidgetsPrivate
    LinguistTools
)

add_subdirectory(ElaWidgetTools)

set(APP_SOURCES
    src/main.cpp
    src/app/MainWindow.h
    src/app/MainWindow.cpp
    src/app/pages/HomePage.h
    src/app/pages/HomePage.cpp
    src/app/pages/ConvertPage.h
    src/app/pages/ConvertPage.cpp
    src/app/pages/ImagePage.h
    src/app/pages/ImagePage.cpp
    src/app/pages/JoinPage.h
    src/app/pages/JoinPage.cpp
    src/app/pages/BatchPage.h
    src/app/pages/BatchPage.cpp
    src/app/pages/QueuePage.h
    src/app/pages/QueuePage.cpp
    src/app/pages/SettingsPage.h
    src/app/pages/SettingsPage.cpp
    src/app/pages/VideoPage.h
    src/app/pages/VideoPage.cpp
    src/ffmpeg/FFmpegLocator.h
    src/ffmpeg/FFmpegLocator.cpp
    src/ffmpeg/FFmpegCapabilities.h
    src/ffmpeg/FFmpegCapabilities.cpp
    src/ffmpeg/FFmpegCommand.h
    src/ffmpeg/FFmpegCommandBuilder.h
    src/ffmpeg/FFmpegCommandBuilder.cpp
    src/ffmpeg/FFmpegProcess.h
    src/ffmpeg/FFmpegProcess.cpp
    src/media/MediaFile.h
    src/media/MediaProbeService.h
    src/media/MediaProbeService.cpp
    src/jobs/Job.h
    src/jobs/Job.cpp
    src/jobs/JobListModel.h
    src/jobs/JobListModel.cpp
    src/jobs/JobQueue.h
    src/jobs/JobQueue.cpp
    src/i18n/LanguageManager.h
    src/i18n/LanguageManager.cpp
    src/i18n/TrId.h
    src/output/OutputNaming.h
    src/output/OutputNaming.cpp
    src/presets/PresetManager.h
    src/presets/PresetManager.cpp
)

set(TS_FILES
    i18n/ffmedia_en.ts
    i18n/ffmedia_zh_CN.ts
)

qt_add_executable(ffmedia-toolbox WIN32
    ${APP_SOURCES}
)

qt_add_translations(ffmedia-toolbox
    TS_FILES ${TS_FILES}
    TS_OUTPUT_DIRECTORY i18n
    LUPDATE_OPTIONS -no-obsolete
    LRELEASE_OPTIONS -idbased
)

target_include_directories(ffmedia-toolbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ffmedia-toolbox PRIVATE
    Qt6::Concurrent
    Qt6::Widgets
    Ela::WidgetTools
)

add_custom_command(TARGET ffmedia-toolbox POST_BUILD
    COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe"
        --no-translations
        --no-opengl-sw
        --no-compiler-runtime
        "$<TARGET_FILE:ffmedia-toolbox>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:ElaWidgetTools>"
        "$<TARGET_FILE_DIR:ffmedia-toolbox>"
    COMMENT "Deploying Qt runtime and ElaWidgetTools..."
)
