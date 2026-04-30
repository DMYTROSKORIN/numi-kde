cmake_minimum_required(VERSION 3.24)

project(numi-kde-native VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
find_package(PkgConfig REQUIRED)
pkg_check_modules(QALCULATE REQUIRED libqalculate)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(numi-kde
  src/main.cpp
  src/documentmodel.cpp
  src/documentmodel.h
  src/qalcbridge.cpp
  src/qalcbridge.h
)

target_compile_definitions(numi-kde PRIVATE NUMI_KDE_SOURCE_DIR="${CMAKE_SOURCE_DIR}/..")
target_include_directories(numi-kde PRIVATE ${QALCULATE_INCLUDE_DIRS})

qt_add_qml_module(numi-kde
  URI NumiKde
  VERSION 1.0
  QML_FILES
    qml/Main.qml
    qml/DocumentPage.qml
    qml/EditorPane.qml
    qml/ResultsPane.qml
)

target_link_libraries(numi-kde
  PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    ${QALCULATE_LIBRARIES}
)

install(TARGETS numi-kde RUNTIME DESTINATION bin)
install(FILES resources/org.skorin.numi-kde.desktop DESTINATION share/applications)
install(FILES resources/org.skorin.numi-kde.metainfo.xml DESTINATION share/metainfo)
