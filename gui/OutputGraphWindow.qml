// Interface QML para o módulo OutputGraphWindow.
import QtQuick
import QtQuick.Controls
import QtQuick.Window

Window {
    id: graphWindow
    width: 720
    height: 440
    minimumWidth: 480
    minimumHeight: 300
    visible: true
    title: "MICROHIL — " + outputName

    property string outputName: ""
    property real yMinimum: 0
    property real yMaximum: 1
    property real yResolution: 0.1
    property real timeWindowSeconds: 10
    property var samples: []
    signal discarded()

    Canvas {
        id: chart
        anchors.fill: parent
        anchors.margins: 24
        onPaint: {
            var context = getContext("2d")
            context.reset()
            context.fillStyle = "#f4f6f5"
            context.fillRect(0, 0, width, height)
            context.strokeStyle = "#b8c7bd"
            context.lineWidth = 1
            var range = graphWindow.yMaximum - graphWindow.yMinimum
            if (range <= 0 || graphWindow.yResolution <= 0) return
            for (var value = graphWindow.yMinimum; value <= graphWindow.yMaximum + graphWindow.yResolution * 0.001; value += graphWindow.yResolution) {
                var y = height - (value - graphWindow.yMinimum) / range * height
                context.beginPath()
                context.moveTo(48, y)
                context.lineTo(width, y)
                context.stroke()
                context.fillStyle = "#37474f"
                context.font = "12px sans-serif"
                context.fillText(Number(value).toString(), 2, y - 3)
            }
            context.strokeStyle = "#2f8f62"
            context.lineWidth = 2
            var startTime = graphWindow.samples.length > 0 ? Math.max(0, graphWindow.samples[graphWindow.samples.length - 1].time - graphWindow.timeWindowSeconds) : 0
            for (var index = 0; index < graphWindow.samples.length; ++index) {
                var sample = graphWindow.samples[index]
                var x = 48 + (sample.time - startTime) / graphWindow.timeWindowSeconds * (width - 48)
                var yValue = height - (sample.value - graphWindow.yMinimum) / range * height
                if (index === 0) context.moveTo(x, yValue)
                else context.lineTo(x, yValue)
            }
            if (graphWindow.samples.length > 0) context.stroke()
        }
    }

    Label {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 12
        text: "Janela temporal: " + timeWindowSeconds + " s"
        color: "#37474f"
    }

    onSamplesChanged: chart.requestPaint()
    onClosing: {
        discarded()
        graphWindow.destroy()
    }
}
