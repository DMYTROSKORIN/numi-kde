// Numi-KDE window memory — KWin script.
//
// On Wayland a client cannot position its own window and KWin's "Remember
// position" rule is only persisted for X11 windows. This script bridges the
// gap: it reports the geometry of Numi-KDE windows to the running app over
// D-Bus whenever the user finishes moving them, and asks the app for the last
// saved geometry when a Numi-KDE window appears.
//
// The app side is `WindowMemory` (kde/src/windowmemory.cpp), exported at
// online.skorin.numi-kde /WindowMemory.

const APP_ID = "online.skorin.numi-kde";
const SERVICE = "online.skorin.numi-kde";
const PATH = "/WindowMemory";
const IFACE = "online.skorin.numi_kde.WindowMemory";

function isOurs(w) {
    return w && String(w.resourceClass) === APP_ID && w.normalWindow;
}

function remember(w) {
    if (!isOurs(w) || w.move || w.resize) return;
    const g = w.frameGeometry;
    callDBus(SERVICE, PATH, IFACE, "rememberGeometry", String(w.caption),
             Math.round(g.x), Math.round(g.y), Math.round(g.width), Math.round(g.height));
}

function restore(w) {
    callDBus(SERVICE, PATH, IFACE, "savedGeometry", String(w.caption), function (reply) {
        // reply: "x,y" or "" when nothing is saved yet
        const parts = String(reply).split(",");
        if (parts.length < 2) return;
        const x = parseInt(parts[0], 10), y = parseInt(parts[1], 10);
        if (isNaN(x) || isNaN(y)) return;
        const g = w.frameGeometry;
        const screen = workspace.virtualScreenGeometry;
        // Never restore to a place that is no longer on any screen.
        if (x < screen.x || y < screen.y
                || x + Math.min(g.width, 64) > screen.x + screen.width
                || y + Math.min(g.height, 64) > screen.y + screen.height) return;
        w.frameGeometry = { x: x, y: y, width: g.width, height: g.height };
    });
}

function track(w) {
    if (!isOurs(w)) return;
    restore(w);
    w.interactiveMoveResizeFinished.connect(function () { remember(w); });
    // Hiding the window (Esc, hotkey, ×) destroys the Wayland toplevel: save on the way out too.
    w.closed.connect(function () { remember(w); });
}

workspace.windowList().forEach(track);
workspace.windowAdded.connect(track);
