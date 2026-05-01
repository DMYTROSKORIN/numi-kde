import assert from "node:assert/strict";
import fs from "node:fs";
import test from "node:test";

const requiredFiles = [
  "kde/CMakeLists.txt",
  "kde/src/main.cpp",
  "kde/qml/Main.qml",
  "kde/qml/DocumentPage.qml",
  "kde/qml/EditorPane.qml",
  "kde/qml/ResultsPane.qml",
  "kde/resources/org.skorin.numi-kde.desktop",
  "kde/resources/org.skorin.numi-kde.metainfo.xml",
  "kde/src/qalcbridge.h",
  "kde/src/qalcbridge.cpp",
  "kde/src/syntaxhighlighter.h",
  "kde/src/syntaxhighlighter.cpp",
];

test("native KDE skeleton files exist", () => {
  for (const file of requiredFiles) {
    assert.equal(fs.existsSync(file), true, `${file} should exist`);
  }
});

test("native KDE CMake skeleton declares required Qt packages and libqalculate", () => {
  const cmake = fs.readFileSync("kde/CMakeLists.txt", "utf8");

  assert.match(cmake, /find_package\(Qt6 6\.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2\)/);
  assert.match(cmake, /pkg_check_modules\(QALCULATE REQUIRED libqalculate\)/);
  assert.match(cmake, /find_package\(KF6WindowSystem REQUIRED\)/);
  assert.match(cmake, /target_link_libraries\(numi-kde\n\s+PRIVATE\n\s+Qt6::Core\n\s+Qt6::Gui\n\s+Qt6::Qml\n\s+Qt6::Quick\n\s+Qt6::QuickControls2\n\s+KF6::WindowSystem\n\s+\$\{QALCULATE_LIBRARIES\}/);
  assert.match(cmake, /qt_add_qml_module/);
  assert.doesNotMatch(cmake, /KF6Kirigami/);
});

test("native KDE QML skeleton contains editor and result panes", () => {
  const documentPage = fs.readFileSync("kde/qml/DocumentPage.qml", "utf8");
  const editorPane = fs.readFileSync("kde/qml/EditorPane.qml", "utf8");
  const resultsPane = fs.readFileSync("kde/qml/ResultsPane.qml", "utf8");

  assert.match(documentPage, /lines: documentModel/);
  assert.match(documentPage, /highlightModel: documentModel/);
  assert.match(documentPage, /documentModel\.source = text/);
  assert.match(documentPage, /text: "⚙"/);
  assert.doesNotMatch(documentPage, /text: "→"/);
  assert.match(editorPane, /Controls\.TextArea/);
  assert.match(editorPane, /highlightedHtml/);
  assert.match(editorPane, /Text\.RichText/);
  assert.match(editorPane, /color: "transparent"/);
  assert.match(editorPane, /cursorDelegate:/);
  assert.match(editorPane, /property int monoSize/);
  assert.match(editorPane, /placeholderText:/);
  assert.doesNotMatch(editorPane, /onCursorPositionChanged/);
  assert.match(resultsPane, /ListView/);
  assert.match(resultsPane, /property Item syncFlickable/);
  assert.match(resultsPane, /property int monoSize/);
  assert.match(resultsPane, /onContentYChanged/);
  assert.match(resultsPane, /width: root\.availableWidth/);
});

test("native KDE backend uses libqalculate bridge and KWindowSystem", () => {
  const modelH = fs.readFileSync("kde/src/documentmodel.h", "utf8");
  const modelCpp = fs.readFileSync("kde/src/documentmodel.cpp", "utf8");
  const bridgeH = fs.readFileSync("kde/src/qalcbridge.h", "utf8");

  assert.match(modelH, /class QalcBridge;/);
  assert.match(modelH, /QalcBridge \*m_qalc;/);
  assert.match(modelCpp, /m_qalc = new QalcBridge\(this\);/);
  assert.match(modelCpp, /m_qalc->evaluateDocument\(m_source\)/);
  assert.match(modelCpp, /KX11Extras::setState/);
  assert.match(modelCpp, /win->setFlag\(Qt::WindowStaysOnTopHint/);
  assert.match(bridgeH, /class Calculator;/);
  assert.match(bridgeH, /evaluateDocument/);
});

test("native KDE QML uses Numi reference visual tokens", () => {
  const main = fs.readFileSync("kde/qml/Main.qml", "utf8");
  const documentPage = fs.readFileSync("kde/qml/DocumentPage.qml", "utf8");
  const resultsPane = fs.readFileSync("kde/qml/ResultsPane.qml", "utf8");

  assert.match(main, /width: windowSettings\.savedWidth/);
  assert.match(main, /height: windowSettings\.savedHeight/);
  assert.match(main, /savedWidth: 456/);
  assert.match(main, /savedHeight: 368/);
  assert.match(main, /FramelessWindowHint/);
  assert.match(main, /WindowStaysOnTopHint/);
  assert.match(main, /Settings/);
  assert.match(main, /ResizeHandle/);
  assert.match(main, /"#22242a"/);
  assert.match(main, /"#ffd35a"/);
  assert.match(main, /"#6fc4e8"/);
  assert.match(main, /"#8fd14f"/);
  assert.match(main, /fontSize/);
  assert.match(main, /resultWidth/);
  assert.match(documentPage, /placeholderText:/);
  assert.match(documentPage, /\/help/);
  assert.match(documentPage, /alwaysOnTop/);
  assert.match(documentPage, /settingsPopup/);
  assert.match(documentPage, /Slider/);
  assert.match(resultsPane, /horizontalAlignment: Text\.AlignRight/);
});

test("native KDE QML uses Linux window controls", () => {
  const main = fs.readFileSync("kde/qml/Main.qml", "utf8");

  assert.match(main, /historyButton/);
  assert.match(main, /showMinimized/);
  assert.match(main, /Window\.Maximized/);
  assert.match(main, /Qt\.quit/);
  assert.doesNotMatch(main, /#febc2e/);
  assert.doesNotMatch(main, /#28c840/);
});

test("native KDE QML wires live input and result copy behavior", () => {
  const documentPage = fs.readFileSync("kde/qml/DocumentPage.qml", "utf8");
  const resultsPane = fs.readFileSync("kde/qml/ResultsPane.qml", "utf8");
  const main = fs.readFileSync("kde/qml/Main.qml", "utf8");

  assert.match(documentPage, /documentModel\.source = text/);
  assert.match(documentPage, /lines: documentModel/);
  assert.match(documentPage, /documentModel\.copyResult\(row\)/);
  assert.match(resultsPane, /hoverEnabled: true/);
  assert.match(resultsPane, /copyRequested\(index\)/);
  assert.match(main, /WindowStaysOnTopHint/);
  assert.match(main, /documentModel\.setKeepAbove/);
  assert.match(main, /savedWidth/);
  assert.match(main, /startSystemResize/);
});
