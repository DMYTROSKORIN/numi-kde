cmake_minimum_required(VERSION 3.24)

project(numi-kde-native VERSION 0.1.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core DBus Gui Widgets Network Qml Quick QuickControls2)
find_package(PkgConfig REQUIRED)
pkg_check_modules(QALCULATE REQUIRED libqalculate)
find_package(KF6WindowSystem REQUIRED)
find_package(KF6GlobalAccel QUIET)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(numi-kde
  src/main.cpp
  src/documentmodel.cpp
  src/documentmodel.h
  src/shortcutmanager.cpp
  src/shortcutmanager.h
  src/qalcbridge.cpp
  src/qalcbridge.h
  src/syntaxhighlighter.cpp
  src/syntaxhighlighter.h
)

target_include_directories(numi-kde PRIVATE ${QALCULATE_INCLUDE_DIRS})

set_source_files_properties(resources/numi-kde.png PROPERTIES QT_RESOURCE_ALIAS numi-kde.png)
set_source_files_properties(resources/numi-kde-tray.png PROPERTIES QT_RESOURCE_ALIAS numi-kde-tray.png)

qt_add_resources(numi-kde "icons"
  PREFIX "/icons"
  FILES
    resources/numi-kde.png
    resources/numi-kde-tray.png
)

qt_add_qml_module(numi-kde
  URI NumiKde
  VERSION 1.0
  QML_FILES
    qml/Main.qml
    qml/DocumentPage.qml
    qml/EditorPane.qml
    qml/ResultsPane.qml
    qml/SettingsWindow.qml
    qml/HistoryPane.qml
)

target_link_libraries(numi-kde
  PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    KF6::WindowSystem
    ${QALCULATE_LIBRARIES}
)
target_compile_definitions(numi-kde PRIVATE NUMI_KDE_VERSION="${PROJECT_VERSION}")

if(KF6GlobalAccel_FOUND)
  target_link_libraries(numi-kde PRIVATE KF6::GlobalAccel)
  target_compile_definitions(numi-kde PRIVATE NUMI_KDE_HAVE_GLOBAL_ACCEL=1)
endif()

install(TARGETS numi-kde RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES resources/org.skorin.numi-kde.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/org.skorin.numi-kde.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES resources/numi-kde.png
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
  RENAME org.skorin.numi-kde.png
)

# ── Math test suite ──────────────────────────────────────────────────────────
enable_testing()

add_executable(numi-kde-tests
  tests/qalc_test.cpp
  src/documentmodel.cpp
  src/qalcbridge.cpp
  src/syntaxhighlighter.cpp
)
target_include_directories(numi-kde-tests PRIVATE src ${QALCULATE_INCLUDE_DIRS})
target_link_libraries(numi-kde-tests
  PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    KF6::WindowSystem
    ${QALCULATE_LIBRARIES}
)
set_target_properties(numi-kde-tests PROPERTIES
  AUTOMOC ON
)

add_test(NAME qalc-math COMMAND numi-kde-tests qalc)
add_test(NAME documentmodel COMMAND numi-kde-tests documentmodel)
set_tests_properties(qalc-math documentmodel PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
