import QtQuick
import QtQuick.Controls
import qs.Commons
import qs.Ui

Column {
  id: root
  property var bar: null
  property color foreground: Color.foreground
  property string fontFamily: Style.font.family
  readonly property color dim: Qt.darker(foreground, 1.45)

  property string saverId: ""
  property string saverName: ""
  property var fields: []
  property var values: ({})
  property int previewSeconds: 12

  signal setValue(string saverId, string key, var value)
  signal setPreviewSeconds(int seconds)
  signal closeRequested()

  spacing: Style.space(10)
  width: parent ? parent.width : Style.space(360)

  Text {
    width: parent.width
    text: "Preview length"
    color: root.dim
    font.family: root.fontFamily
    font.pixelSize: Style.font.caption
  }

  Row {
    width: parent.width
    spacing: Style.space(8)
    PanelSlider {
      id: previewSlider
      bar: root.bar
      width: parent.width - Style.space(48)
      minimum: 3
      maximum: 30
      step: 1
      integer: true
      value: root.previewSeconds
      onReleased: function(v) { root.setPreviewSeconds(Math.round(v)) }
    }
    Text {
      anchors.verticalCenter: parent.verticalCenter
      width: Style.space(40)
      text: Math.round(previewSlider.dragging ? previewSlider.liveValue : root.previewSeconds) + "s"
      color: root.foreground
      font.family: root.fontFamily
      font.pixelSize: Style.font.caption
    }
  }

  Repeater {
    model: root.fields

    Column {
      required property var modelData
      width: root.width
      spacing: Style.space(4)
      visible: modelData && modelData.key

      Text {
        width: parent.width
        text: modelData.label || modelData.key
        color: root.dim
        font.family: root.fontFamily
        font.pixelSize: Style.font.caption
      }

      Row {
        width: parent.width
        spacing: Style.space(8)
        visible: modelData.type === "int" || modelData.type === "float"

        PanelSlider {
          id: fieldSlider
          bar: root.bar
          width: parent.width - Style.space(56)
          minimum: Number(modelData.min)
          maximum: Number(modelData.max)
          step: modelData.type === "int" ? 1 : Number(modelData.step || 0.1)
          integer: modelData.type === "int"
          value: Number(root.values[modelData.key] !== undefined ? root.values[modelData.key] : modelData.default)
          onReleased: function(v) {
            root.setValue(root.saverId, modelData.key, modelData.type === "int" ? Math.round(v) : Math.round(v * 100) / 100)
          }
        }
        Text {
          anchors.verticalCenter: parent.verticalCenter
          width: Style.space(48)
          text: {
            var v = fieldSlider.dragging ? fieldSlider.liveValue : Number(root.values[modelData.key] !== undefined ? root.values[modelData.key] : modelData.default)
            return modelData.type === "int" ? String(Math.round(v)) : v.toFixed(2)
          }
          color: root.foreground
          font.family: root.fontFamily
          font.pixelSize: Style.font.caption
        }
      }

      ToggleSwitch {
        visible: modelData.type === "bool"
        checked: root.values[modelData.key] === true
        onToggled: root.setValue(root.saverId, modelData.key, !root.values[modelData.key])
      }

      TextField {
        visible: modelData.type === "string"
        width: parent.width
        text: String(root.values[modelData.key] !== undefined ? root.values[modelData.key] : (modelData.default || ""))
        color: root.foreground
        font.family: root.fontFamily
        onEditingFinished: root.setValue(root.saverId, modelData.key, text)
      }
    }
  }

  Text {
    visible: root.fields.length === 0
    width: parent.width
    text: "This screensaver has no extra settings."
    color: root.dim
    font.family: root.fontFamily
    font.pixelSize: Style.font.bodySmall
    wrapMode: Text.WordWrap
  }
}
