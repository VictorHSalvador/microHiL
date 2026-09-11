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
    property bool loggingEnabled: true
    property bool plotEnabled: true
    property var outputItems: []
    property var inputItems: []
    property var mappingRows: []
    property string observedFmuPath: guiController.fmuPath

    function refreshOutputs() {
        outputItems = guiController.Outputs()
    }

    function refreshMappings() {
        inputItems = guiController.Inputs()
        var rows = []
        for (var inputIndex = 0; inputIndex < inputItems.length; ++inputIndex) {
            var input = inputItems[inputIndex]
            rows.push({ variable: input.name, type: input.type, typeCode: input.typeCode, direction: "input", channel: "", scale: "", offset: "" })
        }
        for (var outputIndex = 0; outputIndex < outputItems.length; ++outputIndex) {
            var output = outputItems[outputIndex]
            rows.push({ variable: output.name, type: output.type, typeCode: output.typeCode, direction: "output", channel: "", scale: "", offset: "" })
        }
        mappingRows = rows
    }

    function updateMapping(index, key, value) {
        var rows = mappingRows.slice()
        var row = Object.assign({}, rows[index])
        row[key] = value
        rows[index] = row
        mappingRows = rows
    }

    function profileMappings() {
        var result = []
        for (var index = 0; index < mappingRows.length; ++index) {
            var row = mappingRows[index]
            if (row.channel.length === 0) continue
            if (row.scale.length === 0 || row.offset.length === 0) return null
            result.push({ channel: row.channel, variable: row.variable, type: row.type, scale: Number(row.scale), offset: Number(row.offset) })
        }
        return result
    }

    palette.window: "#f4f6f5"
    palette.windowText: "#263238"
    palette.button: "#e5ebe8"
    palette.buttonText: "#263238"
    palette.highlight: "#2f8f62"
    palette.highlightedText: "white"

    FileDialog {
        id: fmuDialog
        nameFilters: ["FMU files (*.fmu)"]
        onAccepted: guiController.LoadFmu(selectedFile.toString().replace("file://", ""))
    }

    FileDialog {
        id: saveProfileDialog
        fileMode: FileDialog.SaveFile
        nameFilters: ["YAML files (*.yaml *.yml)"]
        onAccepted: {
            var mappings = window.profileMappings()
            if (mappings === null) {
                window.profileError = "Informe escala e offset para todo mapeamento físico."
                return
            }
            window.profileError = ""
            guiController.SaveProfile(selectedFile.toString().replace("file://", ""), Number(stepSizeField.text), Number(stopTimeField.text), Number(adcResolutionField.text),
                                      [Number(adc32Field.text), Number(adc33Field.text), Number(adc34Field.text), Number(adc35Field.text), Number(adc36Field.text), Number(adc39Field.text)],
                                      [{ frequency_hz: Number(pwm18FrequencyField.text), resolution_bits: Number(pwm18ResolutionField.text) },
                                       { frequency_hz: Number(pwm19FrequencyField.text), resolution_bits: Number(pwm19ResolutionField.text) }], mappings)
        }
    }

    property string profileError: ""

    FileDialog {
        id: profileDialog
        nameFilters: ["YAML files (*.yaml *.yml)"]
        onAccepted: guiController.LoadProfile(selectedFile.toString().replace("file://", ""))
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
                        Label { text: guiController.fmuPath || "Nenhuma FMU selecionada"; Layout.fillWidth: true; elide: Text.ElideMiddle }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Button { text: "Carregar perfil YAML"; onClicked: profileDialog.open() }
                        Label { text: guiController.profilePath || "Nenhum perfil selecionado"; Layout.fillWidth: true; elide: Text.ElideMiddle }
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
                        TextField { id: stepSizeField; text: "0.01"; validator: DoubleValidator { bottom: 0.000001 } }
                        Label { text: "Duração (s)" }
                        TextField { id: stopTimeField; text: "60"; validator: DoubleValidator { bottom: 0.000001 } }
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
                visible: guiController.fmuPath.length > 0
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "Mapeamento físico ESP32"; font.bold: true; font.pixelSize: 18 }
                    Label { text: "Real pode usar AI, AO ou PWM; Boolean pode usar DI ou DO. Integer e Enumeration permanecem como entrada virtual ou saída de log/gráfico."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Repeater {
                        model: window.mappingRows
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            property var options: [""].concat(guiController.DaqcChannels(modelData.typeCode, modelData.direction === "input"))
                            Label { text: modelData.direction === "input" ? "Entrada" : "Saída"; Layout.preferredWidth: 65 }
                            Label { text: modelData.variable + " (" + modelData.type + ")"; Layout.preferredWidth: 245; elide: Text.ElideRight }
                            ComboBox {
                                Layout.preferredWidth: 170
                                model: parent.options
                                enabled: parent.options.length > 1
                                currentIndex: Math.max(0, parent.options.indexOf(modelData.channel))
                                displayText: currentText.length === 0 ? (enabled ? "Sem DAQC" : "Virtual / log") : currentText
                                onActivated: window.updateMapping(index, "channel", currentText)
                            }
                            TextField {
                                Layout.preferredWidth: 120
                                placeholderText: "Escala"
                                text: modelData.scale
                                enabled: modelData.channel.length > 0
                                validator: DoubleValidator {}
                                onEditingFinished: window.updateMapping(index, "scale", text)
                            }
                            TextField {
                                Layout.preferredWidth: 120
                                placeholderText: "Offset"
                                text: modelData.offset
                                enabled: modelData.channel.length > 0
                                validator: DoubleValidator {}
                                onEditingFinished: window.updateMapping(index, "offset", text)
                            }
                        }
                    }
                }
            }

            Frame {
                Layout.fillWidth: true
                visible: guiController.fmuPath.length > 0
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "Configuração DAQC para YAML"; font.bold: true; font.pixelSize: 18 }
                    Label { text: "Todos os campos ADC/PWM são obrigatórios, inclusive para canais sem mapeamento. A DAQC ainda confirma a combinação PWM antes de Streaming."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    GridLayout {
                        columns: 4
                        Label { text: "Resolução ADC (bits)" }
                        TextField { id: adcResolutionField; placeholderText: "9–12"; validator: IntValidator { bottom: 9; top: 12 } }
                        Label { text: "Atenuação GPIO32" }
                        TextField { id: adc32Field; placeholderText: "0–3"; validator: IntValidator { bottom: 0; top: 3 } }
                        Label { text: "Atenuação GPIO33" }
                        TextField { id: adc33Field; placeholderText: "0–3"; validator: IntValidator { bottom: 0; top: 3 } }
                        Label { text: "Atenuação GPIO34" }
                        TextField { id: adc34Field; placeholderText: "0–3"; validator: IntValidator { bottom: 0; top: 3 } }
                        Label { text: "Atenuação GPIO35" }
                        TextField { id: adc35Field; placeholderText: "0–3"; validator: IntValidator { bottom: 0; top: 3 } }
                        Label { text: "Atenuação GPIO36" }
                        TextField { id: adc36Field; placeholderText: "0–3"; validator: IntValidator { bottom: 0; top: 3 } }
                        Label { text: "Atenuação GPIO39" }
                        TextField { id: adc39Field; placeholderText: "0–3"; validator: IntValidator { bottom: 0; top: 3 } }
                        Label { text: "PWM GPIO18: frequência (Hz)" }
                        TextField { id: pwm18FrequencyField; placeholderText: "> 0"; validator: IntValidator { bottom: 1 } }
                        Label { text: "PWM GPIO18: resolução (bits)" }
                        TextField { id: pwm18ResolutionField; placeholderText: "1–20"; validator: IntValidator { bottom: 1; top: 20 } }
                        Label { text: "PWM GPIO19: frequência (Hz)" }
                        TextField { id: pwm19FrequencyField; placeholderText: "> 0"; validator: IntValidator { bottom: 1 } }
                        Label { text: "PWM GPIO19: resolução (bits)" }
                        TextField { id: pwm19ResolutionField; placeholderText: "1–20"; validator: IntValidator { bottom: 1; top: 20 } }
                    }
                    Button { text: "Salvar perfil YAML"; onClicked: saveProfileDialog.open() }
                    Label { text: window.profileError; color: "#b33a3a"; visible: text.length > 0 }
                }
            }

            Frame {
                Layout.fillWidth: true
                ColumnLayout {
                    anchors.fill: parent
                    Label { text: "Gráficos de saída"; font.bold: true; font.pixelSize: 18 }
                    Label { text: "Abra gráficos por variável após a importação da FMU. Cada janela mantém histórico somente enquanto estiver aberta."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Label { text: guiController.fmuPath ? guiController.modelName + " — " + guiController.inputCount + " entradas, " + guiController.outputCount + " saídas" : "" }
                    Label { text: guiController.profilePath ? "Perfil ESP32 " + guiController.profileId + " — " + guiController.profileMappingCount + " mapeamento(s)" : "" }
                    Label { text: "Saídas selecionadas para log e atuação"; font.bold: true; visible: guiController.fmuPath.length > 0 }
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.min(contentHeight, 180)
                        clip: true
                        model: window.outputItems
                        delegate: CheckBox {
                            required property var modelData
                            text: modelData.name + " — " + modelData.type + " (VR " + modelData.valueReference + ")"
                            checked: modelData.selected
                            onToggled: {
                                guiController.SetOutputSelected(modelData.index, checked)
                                window.refreshOutputs()
                            }
                        }
                    }
                    Label { text: guiController.errorMessage; color: "#b33a3a"; visible: text.length > 0 }
                    Button { text: "Configurar gráficos"; enabled: guiController.fmuPath.length > 0 }
                }
            }
        }
    }

    onObservedFmuPathChanged: {
        refreshOutputs()
        refreshMappings()
    }
}
