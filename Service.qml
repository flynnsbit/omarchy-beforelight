import QtQuick
import Quickshell
import Quickshell.Io

Item {
  id: root
  property var shell: null

  readonly property string pluginDir: {
    var url = Qt.resolvedUrl(".").toString()
    if (url.indexOf("file://") === 0) url = url.substring(7)
    return url.replace(/\/+$/, "")
  }

  Process {
    id: setupProcess
    running: true
    command: ["bash", root.pluginDir + "/scripts/setup.sh", "--quiet"]
  }
}
