import QtQuick
import QtQuick.Controls
import Quickshell
import Quickshell.Io
import qs.Commons
import qs.Ui
import "Model.js" as Model

Panel {
  id: root
  moduleName: "beforelight"
  manageIpc: false

  property var anchorItem: null
  property var hostWidget: null
  readonly property var barIdentity: hostWidget || root

  readonly property color foreground: bar ? bar.foreground : Color.foreground
  readonly property color dim: Qt.darker(foreground, 1.45)
  readonly property string fontFamily: bar ? bar.fontFamily : Style.font.family
  readonly property string pluginDir: {
    var url = Qt.resolvedUrl(".").toString()
    if (url.indexOf("file://") === 0) url = url.substring(7)
    return url.replace(/\/+$/, "")
  }
  readonly property string cli: pluginDir + "/bin/omarchy-beforelight"

  property var items: []
  property string selectedId: ""
  property string engine: "beforelight"
  property string loadError: ""
  property int cursorIndex: 0
  property bool cursorActive: false
  property bool loading: false
  property string previewingId: ""
  property bool settingsOpen: false
  property var settingsFields: []
  property var settingsValues: ({})
  property int previewSeconds: 12

  readonly property var currentItem: {
    for (var i = 0; i < items.length; i++)
      if (items[i].id === selectedId) return items[i]
    return items.length ? items[0] : null
  }

  readonly property var focusedItem: {
    if (items.length === 0) return null
    var idx = Math.max(0, Math.min(cursorIndex, items.length - 1))
    return items[idx]
  }

  function barLabel() {
    if (currentItem && currentItem.emoji) return currentItem.emoji
    return "🍞"
  }

  function tooltipText() {
    if (currentItem)
      return "Before Light · " + currentItem.name + "  (right-click previews)"
    return "Before Light screensaver"
  }

  function open() {
    root.refresh()
    root.controller.show()
  }

  function close() {
    root.controller.hide()
  }

  function toggle() {
    if (root.opened) root.close()
    else root.open()
  }

  function switchPanel(direction) {
    if (root.bar && typeof root.bar.switchPanelFrom === "function")
      return root.bar.switchPanelFrom(root.barIdentity, direction)
    return false
  }

  function refresh() {
    if (listProcess.running) return
    loading = true
    loadError = ""
    listProcess.running = true
  }

  function applyList(raw) {
    var parsed = Model.parseList(raw)
    loading = false
    if (!parsed.ok) {
      loadError = parsed.error
      return
    }
    items = parsed.items
    selectedId = parsed.selected
    engine = parsed.engine
    cursorIndex = Model.indexOf(items, selectedId)
  }

  function moveCursor(delta) {
    if (items.length === 0) return
    cursorActive = true
    cursorIndex = ((cursorIndex + delta) % items.length + items.length) % items.length
    if (saverList.visible) saverList.positionViewAtIndex(cursorIndex, ListView.Contain)
  }

  function selectFocused() {
    if (!focusedItem) return
    selectSaver(focusedItem.id)
  }

  property bool previewAfterSelect: false

  function selectSaver(id, thenPreview) {
    if (!id || setProcess.running) return
    previewAfterSelect = thenPreview === true
    setProcess.command = [root.cli, "set", id]
    setProcess.running = true
  }

  function previewOnSaver() {
    // Never pass a list-row id. The CLI reads the persisted ON selection.
    if (previewProcess.running) stopProcess.running = true
    previewingId = selectedId
    root.close()
    previewProcess.command = [root.cli, "preview"]
    previewProcess.running = true
  }

  function previewCurrent() {
    previewOnSaver()
  }

  function stopPreview() {
    previewingId = ""
    stopProcess.running = true
  }

  function openSettings() {
    settingsOpen = true
    loadSettings()
  }

  function closeSettings() {
    settingsOpen = false
  }

  function loadSettings() {
    if (!selectedId || selectedId === "omarchy") {
      settingsFields = []
      settingsValues = ({})
      schemaProcess.command = [root.cli, "schema"]
    } else {
      schemaProcess.command = [root.cli, "schema", selectedId]
    }
    schemaProcess.running = true
  }

  function applySchema(raw) {
    try {
      var parsed = JSON.parse(String(raw || ""))
      previewSeconds = Number(parsed.previewSeconds || 12)
      settingsFields = parsed.fields || []
      settingsValues = parsed.values || ({})
    } catch (e) {
      settingsFields = []
    }
  }

  function setSetting(saverId, key, value) {
    if (!saverId || !key) return
    setSettingProcess.command = [
      root.cli,
      "set-setting", saverId, String(key), String(value)
    ]
    setSettingProcess.running = true
  }

  function setPreviewSeconds(seconds) {
    setSettingProcess.command = [
      root.cli,
      "set-setting", "_global", "previewSeconds", String(seconds)
    ]
    setSettingProcess.running = true
  }

  Process {
    id: listProcess
    running: false
    command: [root.cli, "list"]
    stdout: StdioCollector {
      waitForEnd: true
      onStreamFinished: root.applyList(text)
    }
    onExited: function(code) {
      if (code !== 0 && root.items.length === 0)
        root.loadError = "Could not list BeforeLight screensavers."
      root.loading = false
    }
  }

  Process {
    id: setProcess
    running: false
    stdout: StdioCollector {
      waitForEnd: true
      onStreamFinished: root.refresh()
    }
    onExited: {
      Qt.callLater(root.refresh)
      if (root.previewAfterSelect) {
        root.previewAfterSelect = false
        Qt.callLater(root.previewOnSaver)
      }
    }
  }

  Process {
    id: previewProcess
    running: false
  }

  Process {
    id: stopProcess
    running: false
    command: [root.cli, "stop"]
  }

  Process {
    id: schemaProcess
    running: false
    stdout: StdioCollector {
      waitForEnd: true
      onStreamFinished: root.applySchema(text)
    }
  }

  Process {
    id: setSettingProcess
    running: false
    stdout: StdioCollector {
      waitForEnd: true
      onStreamFinished: root.applySchema(text)
    }
  }

  Component.onCompleted: root.refresh()

  KeyboardPanel {
    id: panel
    anchorItem: root.anchorItem
    owner: root.barIdentity
    bar: root.bar
    open: root.opened
    focusTarget: keyCatcher
    contentWidth: panel.fittedContentWidth(Style.space(380))
    contentHeight: panel.fittedContentHeight(column.implicitHeight, Style.space(620))

    PanelKeyCatcher {
      id: keyCatcher
      anchors.fill: parent
      blocked: root.settingsOpen
      onMoveRequested: function(dx, dy) {
        if (root.settingsOpen) return
        if (dy !== 0) root.moveCursor(dy)
        else if (dx !== 0) root.moveCursor(dx)
      }
      onActivateRequested: if (!root.settingsOpen) root.selectFocused()
      onCloseRequested: root.settingsOpen ? root.closeSettings() : root.close()
      onTabRequested: function(direction) { root.switchPanel(direction) }
      onTextKey: function(text) {
        if (root.settingsOpen) return
        if (text === "p" || text === "P") root.previewCurrent()
        else if (text === "s" || text === "S") root.openSettings()
        else if (text === "r" || text === "R") root.refresh()
      }

      Column {
        id: column
        width: parent.width
        spacing: Style.space(12)

        Column {
          visible: !root.settingsOpen
          width: parent.width
          spacing: Style.space(4)

          Text {
            text: "Before Light"
            color: root.foreground
            font.family: root.fontFamily
            font.pixelSize: Style.font.title
            font.bold: true
          }

          Row {
            width: parent.width
            spacing: Style.space(10)

            Text {
              id: currentName
              text: root.currentItem ? root.currentItem.name : "None"
              color: root.foreground
              font.family: root.fontFamily
              font.pixelSize: Style.font.body
              font.bold: true
            }

            Text {
              width: Math.max(0, parent.width - currentName.width - parent.spacing)
              text: root.loadError !== ""
                ? root.loadError
                : (root.currentItem ? root.currentItem.blurb : "")
              color: root.dim
              font.family: root.fontFamily
              font.pixelSize: Style.font.body
              elide: Text.ElideRight
            }
          }
        }

        Column {
          visible: root.settingsOpen
          width: parent.width
          spacing: Style.space(4)

          Item {
            width: parent.width
            height: Math.max(settingsTitle.implicitHeight, settingsBack.implicitHeight)

            Text {
              id: settingsTitle
              anchors.left: parent.left
              anchors.right: settingsBack.left
              anchors.rightMargin: Style.space(8)
              anchors.verticalCenter: parent.verticalCenter
              text: "Settings"
              color: root.foreground
              font.family: root.fontFamily
              font.pixelSize: Style.font.title
              font.bold: true
              elide: Text.ElideRight
            }

            PanelActionButton {
              id: settingsBack
              anchors.right: parent.right
              anchors.verticalCenter: parent.verticalCenter
              iconText: "󰁍"
              tooltipText: "Back"
              foreground: root.foreground
              fontFamily: root.fontFamily
              onClicked: root.closeSettings()
            }
          }

          Text {
            width: parent.width
            text: root.currentItem ? root.currentItem.name : ""
            color: root.foreground
            font.family: root.fontFamily
            font.pixelSize: Style.font.body
            font.bold: true
          }

          Text {
            width: parent.width
            text: "Changes apply to the next preview or idle."
            color: root.dim
            font.family: root.fontFamily
            font.pixelSize: Style.font.caption
            wrapMode: Text.WordWrap
          }
        }

        Text {
          width: parent.width
          visible: root.loading && !root.settingsOpen
          text: "Loading…"
          color: root.dim
          font.family: root.fontFamily
          font.pixelSize: Style.font.bodySmall
        }

        SettingsView {
          visible: root.settingsOpen
          width: parent.width
          bar: root.bar
          foreground: root.foreground
          fontFamily: root.fontFamily
          saverId: root.selectedId
          saverName: root.currentItem ? root.currentItem.name : ""
          fields: root.settingsFields
          values: root.settingsValues
          previewSeconds: root.previewSeconds
          onSetValue: function(id, key, value) { root.setSetting(id, key, value) }
          onSetPreviewSeconds: function(seconds) { root.setPreviewSeconds(seconds) }
          onCloseRequested: root.closeSettings()
        }

        ListView {
          id: saverList
          visible: !root.settingsOpen
          width: parent.width
          height: Math.min(Style.space(320), Math.max(Style.space(120), count * Style.space(36)))
          clip: true
          model: root.items
          currentIndex: root.cursorIndex
          boundsBehavior: Flickable.StopAtBounds
          ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

          delegate: Item {
            required property var modelData
            required property int index
            width: saverList.width
            height: Style.space(36)

            readonly property bool isSelected: modelData.id === root.selectedId
            readonly property bool isFocused: index === root.cursorIndex && root.cursorActive
            readonly property bool showCog: isSelected && modelData.hasSettings === true

            Rectangle {
              anchors.fill: parent
              radius: Style.space(6)
              color: isSelected
                ? Style.selectedFillFor(root.foreground, Color.accent)
                : (isFocused
                  ? Qt.rgba(root.foreground.r, root.foreground.g, root.foreground.b, 0.08)
                  : "transparent")
            }

            Text {
              id: rowLabel
              anchors.left: parent.left
              anchors.leftMargin: Style.space(8)
              anchors.right: rowActions.left
              anchors.rightMargin: Style.space(8)
              anchors.verticalCenter: parent.verticalCenter
              text: (modelData.emoji ? modelData.emoji + "  " : "") + modelData.name
              color: root.foreground
              font.family: root.fontFamily
              font.pixelSize: Style.font.body
              font.bold: isSelected
              elide: Text.ElideRight
            }

            Row {
              id: rowActions
              visible: isSelected
              anchors.right: parent.right
              anchors.rightMargin: Style.space(8)
              anchors.verticalCenter: parent.verticalCenter
              spacing: Style.space(2)

              PanelActionButton {
                iconText: "󰐊"
                tooltipText: "Preview"
                foreground: root.foreground
                fontFamily: root.fontFamily
                onClicked: root.previewCurrent()
              }
              PanelActionButton {
                iconText: "󰓛"
                tooltipText: "Stop preview"
                foreground: root.foreground
                fontFamily: root.fontFamily
                onClicked: root.stopPreview()
              }
              PanelActionButton {
                visible: showCog
                iconText: "󰒓"
                tooltipText: "Settings"
                foreground: root.foreground
                fontFamily: root.fontFamily
                onClicked: root.openSettings()
              }
              Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "ON"
                color: root.dim
                font.family: root.fontFamily
                font.pixelSize: Style.font.caption
                font.bold: true
                font.letterSpacing: 1.1
              }
            }

            MouseArea {
              anchors.fill: parent
              hoverEnabled: true
              z: -1
              onEntered: {
                root.cursorActive = true
                root.cursorIndex = index
              }
              onClicked: root.selectSaver(modelData.id)
              onDoubleClicked: root.selectSaver(modelData.id, true)
            }
          }
        }

        Row {
          visible: !root.settingsOpen
          width: parent.width
          spacing: Style.space(8)

          PanelActionButton {
            iconText: "󰌾"
            tooltipText: "Use this screensaver"
            foreground: root.foreground
            fontFamily: root.fontFamily
            onClicked: root.selectFocused()
          }

          Text {
            anchors.verticalCenter: parent.verticalCenter
            text: "Enter to turn on · P preview"
            color: root.dim
            font.family: root.fontFamily
            font.pixelSize: Style.font.caption
          }
        }
      }
    }
  }
}
