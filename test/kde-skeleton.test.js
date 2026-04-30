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

test("native KDE CMake skeleton declares required Qt packages", () => {
  const cmake = fs.readFileSync("kde/CMakeLists.txt", "utf8");

  assert.match(cmake, /find_package\(Qt6 6\.6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2\)/);
  assert.match(cmake, /qt_add_qml_module/);
  assert.doesNotMatch(cmake, /KF6Kirigami/);
});

test("native KDE QML skeleton contains editor and result panes", () => {
  const documentPage = fs.readFileSync("kde/qml/DocumentPage.qml", "utf8");
  const editorPane = fs.readFileSync("kde/qml/EditorPane.qml", "utf8");
  const resultsPane = fs.readFileSync("kde/qml/ResultsPane.qml", "utf8");

  assert.match(documentPage, /resultLines/);
  assert.match(documentPage, /sampleLines/);
  assert.match(editorPane, /Text\.RichText/);
  assert.match(resultsPane, /ListView/);
  assert.match(resultsPane, /syncFlickable/);
});

test("native KDE QML uses Numi reference visual tokens", () => {
  const main = fs.readFileSync("kde/qml/Main.qml", "utf8");
  const documentPage = fs.readFileSync("kde/qml/DocumentPage.qml", "utf8");
  const resultsPane = fs.readFileSync("kde/qml/ResultsPane.qml", "utf8");

  assert.match(main, /width: 456/);
  assert.match(main, /height: 368/);
  assert.match(main, /FramelessWindowHint/);
  assert.match(main, /"#22242a"/);
  assert.match(main, /"#ffd35a"/);
  assert.match(main, /"#6fc4e8"/);
  assert.match(main, /"#8fd14f"/);
  assert.match(documentPage, /Price:/);
  assert.match(resultsPane, /horizontalAlignment: Text\.AlignRight/);
});
