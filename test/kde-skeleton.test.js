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
];

test("native KDE skeleton files exist", () => {
  for (const file of requiredFiles) {
    assert.equal(fs.existsSync(file), true, `${file} should exist`);
  }
});

test("native KDE CMake skeleton declares required Qt and Kirigami packages", () => {
  const cmake = fs.readFileSync("kde/CMakeLists.txt", "utf8");

  assert.match(cmake, /find_package\(Qt6 6\.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2\)/);
  assert.match(cmake, /find_package\(KF6 REQUIRED COMPONENTS Kirigami\)/);
  assert.match(cmake, /qt_add_qml_module/);
});

test("native KDE QML skeleton contains editor and result panes", () => {
  const documentPage = fs.readFileSync("kde/qml/DocumentPage.qml", "utf8");
  const editorPane = fs.readFileSync("kde/qml/EditorPane.qml", "utf8");
  const resultsPane = fs.readFileSync("kde/qml/ResultsPane.qml", "utf8");

  assert.match(documentPage, /Controls\.SplitView/);
  assert.match(editorPane, /Controls\.TextArea/);
  assert.match(resultsPane, /ListView/);
  assert.match(resultsPane, /syncFlickable/);
});
