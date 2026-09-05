cmake_minimum_required(VERSION 3.24)

project(numi-kde-native VERSION 0.1.80 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

find_package(Qt6 6.6 REQUIRED COMPONENTS Core DBus Gui Widgets Network Qml Quick QuickControls2 Concurrent Test)
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
  src/updatechecker.cpp
  src/updatechecker.h
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
    qml/SettingsPane.qml
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
    Qt6::Concurrent
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

# ── Static Analysis via Semgrep ──────────────────────────────────────────────
find_program(SEMGREP_BIN semgrep)
if(SEMGREP_BIN)
  add_custom_target(semgrep-check
    COMMAND ${SEMGREP_BIN} scan --config ${CMAKE_SOURCE_DIR}/../.semgrep.yml --config auto --error --exclude "build*" --exclude "third_party" ${CMAKE_SOURCE_DIR}/..
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..
    COMMENT "Running Semgrep static analysis..."
  )
  add_dependencies(numi-kde semgrep-check)
endif()

install(TARGETS numi-kde RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES resources/numi-kde.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES resources/numi-kde.metainfo.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES resources/numi-kde.png
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
  RENAME numi-kde.png
)
install(FILES ../LICENSE ../NOTICE
  DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/numi-kde
)
install(PROGRAMS resources/numi-kde-install-update.sh
  DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
  RENAME numi-kde-install-update
)
install(FILES resources/online.skorin.numi-kde-update.policy
  DESTINATION ${CMAKE_INSTALL_DATADIR}/polkit-1/actions
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
    Qt6::Concurrent
    Qt6::Widgets
    Qt6::Test
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    KF6::WindowSystem
    ${QALCULATE_LIBRARIES}
)
target_compile_definitions(numi-kde-tests PRIVATE
  NUMI_KDE_QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/qml"
)
set_target_properties(numi-kde-tests PROPERTIES
  AUTOMOC ON
)

add_test(NAME qalc-math COMMAND numi-kde-tests qalc)
add_test(NAME documentmodel COMMAND numi-kde-tests documentmodel)
add_test(NAME editor-keyhandlers COMMAND numi-kde-tests editor)
set_tests_properties(qalc-math documentmodel editor-keyhandlers PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ── Packaging ────────────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "numi-kde")
set(CPACK_PACKAGE_VENDOR "DMYTRO SKORIN")
set(CPACK_PACKAGE_CONTACT "dev@skorin.online")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "KDE-native document calculator")
set(CPACK_PACKAGE_DESCRIPTION "numi-kde is a KDE-native document-style calculator with support for unit conversion, currency exchange, date arithmetic, and variables.")
set(CPACK_RPM_PACKAGE_DESCRIPTION "numi-kde is a KDE-native document-style calculator with support for unit conversion, currency exchange, date arithmetic, and variables.")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/DMYTROSKORIN/numi-kde")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/../LICENSE")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")

set(CPACK_GENERATOR "RPM")

set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_RPM_PACKAGE_REQUIRES "libqalculate, qt6-qtbase, qt6-qtbase-gui, qt6-qtdeclarative, kf6-kwindowsystem, polkit, dnf")
if(KF6GlobalAccel_FOUND)
  string(APPEND CPACK_RPM_PACKAGE_REQUIRES ", kf6-kglobalaccel")
endif()

include(CPack)
