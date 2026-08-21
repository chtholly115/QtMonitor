cmake_minimum_required(VERSION 3.19)
project(QtMonitor LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Widgets
    LinguistTools
    Multimedia
    OpenGLWidgets
    Concurrent
)

qt_standard_project_setup()

qt_add_executable(QtMonitor
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    openmediadialog.h openmediadialog.cpp openmediadialog.ui
    detector.h detector.cpp
    openglwidget.h openglwidget.cpp
    mediasourcemanager.h mediasourcemanager.cpp
    tracker.h tracker.cpp
    detection.h
)

qt_add_translations(
    TARGETS QtMonitor
    TS_FILES QtMonitor_zh_CN.ts
)

set(ONNXRUNTIME_DIR ${CMAKE_SOURCE_DIR}/onnxruntime)

target_include_directories(QtMonitor
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${ONNXRUNTIME_DIR}/include
)

target_link_directories(QtMonitor PRIVATE
    ${ONNXRUNTIME_DIR}/lib
)

target_link_libraries(QtMonitor
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Multimedia
        Qt::OpenGLWidgets
        Qt::Concurrent
        onnxruntime
)

install(TARGETS QtMonitor
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET QtMonitor
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
