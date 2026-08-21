import QtQuick
import Quickshell
import Quickshell.Io
import qs.Commons
import qs.Ui

BarWidget {
  id: root
  moduleName: "beforelight"

  // FileView cannot watch a path that does not exist yet, so probe the
  // stamp/binary and watch directories that already exist.
  readonly property string homeDir: Quickshell.env("HOME")
  readonly property string stampPath: homeDir + "/.config/omarchy/beforelight.setup"
  readonly property string stampDir: homeDir + "/.config/omarchy"
  readonly property string saverDir: homeDir + "/.config/omarchy/branding/screensaver"
  readonly property string toasterPath: saverDir + "/toastersaver"
  property bool setupReady: false

  readonly property var panelItem: panelLoader.item
  readonly property bool opened: panelItem ? panelItem.opened === true : false
  readonly property bool popoutSwitchClosing: panelItem
    ? panelItem.popoutSwitchClosing === true
    : false

  function probeStamp() {
    if (stampProbe.running) return
    stampProbe.running = true
  }

  function markReady() {
    if (root.setupReady) return
    root.setupReady = true
  }

  function open() {
    if (!root.setupReady) return
    if (panelItem) panelItem.open()
  }

  function close() {
    if (panelItem) panelItem.close()
  }

  function toggle() {
    if (!root.setupReady) return
    if (panelItem) {
      panelItem.refresh()
      panelItem.toggle()
    }
  }

  function closeForPopoutSwitch() {
    if (panelItem) panelItem.closeForPopoutSwitch()
  }

  function refresh() {
    if (panelItem) panelItem.refresh()
  }

  function injectPanel() {
    var target = panelItem
    if (!target) return
    if ("bar" in target) target.bar = root.bar
    if ("settings" in target) target.settings = root.settings
    if ("anchorItem" in target) target.anchorItem = button
    if ("hostWidget" in target) target.hostWidget = root
  }

  implicitWidth: button.implicitWidth
  implicitHeight: button.implicitHeight

  onBarChanged: injectPanel()
  onSettingsChanged: injectPanel()

  Process {
    id: stampProbe
    running: true
    command: ["test", "-s", root.stampPath, "-a", "-x", root.toasterPath]
    onExited: function(code) {
      if (code === 0) root.markReady()
    }
  }

  FileView {
    path: root.stampDir
    watchChanges: true
    printErrors: false
    onFileChanged: if (!root.setupReady) root.probeStamp()
  }

  FileView {
    path: root.saverDir
    watchChanges: true
    printErrors: false
    onFileChanged: if (!root.setupReady) root.probeStamp()
  }

  Timer {
    running: !root.setupReady
    interval: 1500
    repeat: true
    onTriggered: root.probeStamp()
  }

  // Do not leave the bar stuck on "compiling" if gcc/make never finish.
  Timer {
    running: !root.setupReady
    interval: 300000
    repeat: false
    onTriggered: root.markReady()
  }

  Loader {
    id: panelLoader
    active: root.setupReady
    source: Qt.resolvedUrl("Panel.qml")
    visible: false
    onLoaded: {
      root.injectPanel()
      Qt.callLater(root.injectPanel)
    }
  }

  WidgetButton {
    id: button
    anchors.fill: parent
    bar: root.bar
    text: root.setupReady
      ? (root.panelItem ? root.panelItem.barLabel() : "🅰️")
      : "✨"
    tooltipText: root.setupReady
      ? (root.panelItem ? root.panelItem.tooltipText() : "Before Light screensaver")
      : "Compiling screensavers…"
    horizontalMargin: 8.5

    onPressed: function(buttonCode) {
      if (!root.setupReady) return
      if (buttonCode === Qt.RightButton) {
        if (root.panelItem) root.panelItem.previewCurrent()
      } else if (buttonCode === Qt.MiddleButton) {
        if (root.panelItem) root.panelItem.previewCurrent()
      } else {
        root.toggle()
      }
    }
  }
}
