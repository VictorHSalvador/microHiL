import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1120
    height: 720
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: "MICROHIL"

    property string simulationState: "Idle"
    property string fmuPath: ""
    property string profilePath: ""
    property bool loggingEnabled: true
    property bool plotEnabled: true

    palette.window: "#f4f6f5"
    palette.windowText: "#263238"
    palette.button: "#e5ebe8"
    palette.buttonText: "#263238"
    palette.highlight: "#2f8f62"
    palette.highlightedText: "white"

    FileDialog {
        id: fmuDialog
        nameFilters: ["FMU files (*.fmu)"]
        onAccepted: window.fmuPath = selectedFile.toString().replace("file://", "")
    }

    FileDialog {
        id: profileDialog
        nameFilters: ["YAML files (*.yaml *.yml)"]
        onAccepted: window.profilePath = selectedFile.toString().replace("file://", "")
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            Label { text: "MICROHIL"; font.bold: true; font.pixelSize: 20; color: "#246b4b" }
            Item { Layout.fillWidth: true }
            Label { text: "Estado: " + window.simulationState; color: window.simulationState === "Error" ? "#b33a3a" : "#263238" }
        }
    }

    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            Label { text: "A GUI solicita operações ao núcleo C; não executa FMU nem I/O da DAQC diretamente."; color: "#546e5d" }
        }
    }

    ScrollView {
        anchors.fill: parent
        clip: true
        ColumnLayout {
            width: Math.max(window.width - 48, 852)
            anchors.margins: 24
            spacing: 16

            Frame {
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "Modelo e perfil"; font.bold: true; font.pixelSize: 18 }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "Importar FMU"; onClicked: fmuDialog.open() }
                        Label { text: window.fmuPath || "Nenhuma FMU selecionada"; Layout.fillWidth: true; elide: Text.ElideMiddle }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "Carregar perfil YAML"; onClicked: profileDialog.open() }
                        Label { text: window.profilePath || "Nenhum perfil selecionado"; Layout.fillWidth: true; elide: Text.ElideMiddle }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Frame {
                    Layout.fillWidth: true
                    GridLayout {
                        anchors.fill: parent
                        columns: 2
                        Label { text: "Execução"; font.bold: true; font.pixelSize: 18; Layout.columnSpan: 2 }
                        Label { text: "Passo (s)" }
                        TextField { text: "0.01"; validator: DoubleValidator { bottom: 0.000001 } }
                        Label { text: "Duração (s)" }
                        TextField { text: "60"; validator: DoubleValidator { bottom: 0.000001 } }
                        CheckBox { text: "Registrar saídas"; checked: window.loggingEnabled; onToggled: window.loggingEnabled = checked; Layout.columnSpan: 2 }
                        CheckBox { text: "Atualizar gráficos"; checked: window.plotEnabled; onToggled: window.plotEnabled = checked; Layout.columnSpan: 2 }
                    }
                }
                Frame {
                    Layout.fillWidth: true
                    ColumnLayout {
                        anchors.fill: parent
                        Label { text: "Controle"; font.bold: true; font.pixelSize: 18 }
                        RowLayout {
                            Button { text: "Play"; enabled: window.simulationState !== "Running"; onClicked: window.simulationState = "Ready" }
                            Button { text: "Stop"; enabled: window.simulationState === "Running"; onClicked: window.simulationState = "Stopped" }
                        }
                        Label { text: "A ligação de Play/Stop ao controlador de execução C será disponibilizada pela API de orquestração."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "Gráficos de saída"; font.bold: true; font.pixelSize: 18 }
                    Label { text: "Abra gráficos por variável após a importação da FMU. Cada janela mantém histórico somente enquanto estiver aberta."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Button { text: "Configurar gráficos"; enabled: window.fmuPath.length > 0 }
                }
            }
        }
    }
}
