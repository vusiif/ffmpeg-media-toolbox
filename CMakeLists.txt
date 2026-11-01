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
    elseif(EXISTS "C:/Qt/6.12.0/msvc2022_64")
        set(CMAKE_PREFIX_PATH "C:/Qt/6.12.0/msvc2022_64")
    elseif(EXISTS "C:/Qt/6.11.1/msvc2022_64")
        set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
    endif()
endif()

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS
    Concurrent
    Multimedia
    MultimediaWidgets
    Test
    Widgets
    WidgetsPrivate
    LinguistTools
)

add_subdirectory(ElaWidgetTools)

set(FFMEDIA_CORE_SOURCES
    src/app/RecentFiles.h
    src/app/RecentFiles.cpp
    src/conversion/RemuxPolicy.h
    src/conversion/RemuxPolicy.cpp
    src/output/OutputEstimate.h
    src/output/OutputEstimate.cpp
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
    src/media/CorruptedVideoScanner.h
    src/media/CorruptedVideoScanner.cpp
    src/repair/UntruncLocator.h
    src/repair/UntruncLocator.cpp
    src/repair/UntruncProcess.h
    src/repair/UntruncProcess.cpp
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

set(APP_SOURCES
    src/main.cpp
    src/app/MainWindow.h
    src/app/MainWindow.cpp
    src/app/widgets/DamageTimelineSlider.h
    src/app/widgets/DamageTimelineSlider.cpp
    src/app/widgets/VideoPlayerWidget.h
    src/app/widgets/VideoPlayerWidget.cpp
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
    src/app/pages/RepairPage.h
    src/app/pages/RepairPage.cpp
    src/app/pages/SettingsPage.h
    src/app/pages/SettingsPage.cpp
    src/app/pages/VideoPage.h
    src/app/pages/VideoPage.cpp
    src/app/pages/AudioPage.h
    src/app/pages/AudioPage.cpp
    src/app/dialogs/ABCompareDialog.h
    src/app/dialogs/ABCompareDialog.cpp
)

set(TS_FILES
    i18n/ffmedia_en.ts
    i18n/ffmedia_zh_CN.ts
)

add_library(ffmedia-core STATIC ${FFMEDIA_CORE_SOURCES})

target_include_directories(ffmedia-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(ffmedia-core PUBLIC
    Qt6::Concurrent
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Widgets
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

target_link_libraries(ffmedia-toolbox PRIVATE
    ffmedia-core
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

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Others/untrunc/untrunc_x64")
    add_custom_command(TARGET ffmedia-toolbox POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Others/untrunc/untrunc_x64"
            "$<TARGET_FILE_DIR:ffmedia-toolbox>/untrunc_x64"
        COMMENT "Deploying untrunc..."
    )
else()
    message(STATUS "Others/untrunc/untrunc_x64 not found; clean build will continue without bundled untrunc.")
endif()

enable_testing()

foreach(test_name IN ITEMS FFmpegCommandBuilder FFmpegProcess JobListModel OutputEstimate OutputNaming Parsers RecentFiles RemuxPolicy Job JobQueue UntruncProcess CorruptedVideoScanner)
    add_executable(tst_${test_name} tests/tst_${test_name}.cpp)
    target_link_libraries(tst_${test_name} PRIVATE ffmedia-core Qt6::Test)
    add_test(NAME ${test_name} COMMAND tst_${test_name})
    add_custom_command(TARGET tst_${test_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Qt6::Test>"
            "$<TARGET_FILE:Qt6::Concurrent>"
            "$<TARGET_FILE_DIR:tst_${test_name}>"
        COMMENT "Deploying Qt runtime for tst_${test_name}...")
endforeach()
