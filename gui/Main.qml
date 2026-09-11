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
    property var graphConfigurations: ({})
    property var graphWindows: ({})
    property string observedFmuPath: guiController.fmuPath
    property string observedProfilePath: guiController.profilePath

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

    function graphConfiguration(index) {
        return graphConfigurations[index] || { minimum: "", maximum: "", resolution: "" }
    }

    function openGraph(index, name) {
        var configuration = graphConfiguration(index)
        var minimum = Number(configuration.minimum)
        var maximum = Number(configuration.maximum)
        var resolution = Number(configuration.resolution)
        var timeWindow = Number(graphTimeWindowField.text)
        if (!isFinite(minimum) || !isFinite(maximum) || !isFinite(resolution) || !isFinite(timeWindow) || maximum <= minimum || resolution <= 0 || timeWindow <= 0) {
            profileError = "Configure mínimo, máximo, resolução e janela temporal antes de abrir o gráfico."
            return
        }
        profileError = ""
        var component = Qt.createComponent("qrc:/gui/OutputGraphWindow.qml")
        if (component.status === Component.Ready) {
            var graph = component.createObject(window, { outputName: name, yMinimum: minimum, yMaximum: maximum, yResolution: resolution, timeWindowSeconds: timeWindow })
            var windows = Object.assign({}, graphWindows)
            windows[name] = graph
            graphWindows = windows
        }
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
                            Button { text: "Play debug"; enabled: window.simulationState !== "Running"; onClicked: { if (guiController.StartSimulation(Number(stepSizeField.text), Number(stopTimeField.text), window.loggingEnabled, window.plotEnabled)) window.simulationState = "Running" } }
                            Button { text: "Stop"; enabled: window.simulationState === "Running"; onClicked: guiController.StopSimulation() }
                        }
                        Label { text: "Play debug executa a FMU sem DAQC. O Play HiL, com ENABLE→STREAMING e atuação física, depende da integração do enlace."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
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
                    Label { text: "Entradas virtuais"; font.bold: true; font.pixelSize: 18 }
                    Label { text: "O valor é operacional e não é salvo no YAML. A sessão o aplica antes do primeiro passo ou na fronteira do próximo passo, sem chamada FMI pela GUI."; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                    Repeater {
                        model: window.inputItems
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            visible: !modelData.physicalMapped
                            Label { text: modelData.name + " (" + modelData.type + ")"; Layout.preferredWidth: 320; elide: Text.ElideRight }
                            TextField {
                                id: virtualValueField
                                Layout.preferredWidth: 180
                                placeholderText: modelData.type === "Boolean" ? "0 ou 1" : "Valor"
                                validator: DoubleValidator {}
                            }
                            Button { text: "Aplicar"; onClicked: guiController.SetVirtualInput(modelData.index, virtualValueField.text) }
                        }
                    }
                    Label { text: guiController.profilePath.length > 0 ? "Entradas ligadas à DAQC não são exibidas como virtuais." : "Carregue ou salve um perfil para identificar entradas físicas."; color: "#546e5d" }
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
                    RowLayout {
                        Label { text: "Janela temporal comum (s)" }
                        TextField { id: graphTimeWindowField; placeholderText: "> 0"; validator: DoubleValidator { bottom: 0.000001 } Layout.preferredWidth: 140 }
                    }
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
                    Repeater {
                        model: window.outputItems
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            Label { text: modelData.name + " — " + modelData.type; Layout.fillWidth: true }
                            Button {
                                text: "Configurar gráfico"
                                onClicked: {
                                    var configuration = window.graphConfiguration(modelData.index)
                                    graphOutputIndex = modelData.index
                                    graphOutputName = modelData.name
                                    graphMinimumField.text = configuration.minimum
                                    graphMaximumField.text = configuration.maximum
                                    graphResolutionField.text = configuration.resolution
                                    graphConfigurationDialog.open()
                                }
                            }
                            Button { text: "Abrir gráfico"; onClicked: window.openGraph(modelData.index, modelData.name) }
                        }
                    }
                    Label { text: guiController.errorMessage; color: "#b33a3a"; visible: text.length > 0 }
                }
            }
        }
    }

    onObservedFmuPathChanged: {
        refreshOutputs()
        refreshMappings()
    }

    onObservedProfilePathChanged: refreshMappings()

    Timer {
        interval: 100
        running: true
        repeat: true
        onTriggered: {
            var samples = guiController.PollSamples()
            for (var sampleIndex = 0; sampleIndex < samples.length; ++sampleIndex) {
                var sample = samples[sampleIndex]
                for (var name in window.graphWindows) {
                    var graph = window.graphWindows[name]
                    if (graph && sample.values[name] !== undefined) {
                        var values = graph.samples.slice()
                        values.push({ time: sample.time, value: sample.values[name] })
                        var start = sample.time - graph.timeWindowSeconds
                        while (values.length > 0 && values[0].time < start) values.shift()
                        graph.samples = values
                    }
                }
            }
            if (window.simulationState === "Running" && !guiController.SimulationRunning()) window.simulationState = "Finished"
        }
    }

    property int graphOutputIndex: -1
    property string graphOutputName: ""

    Dialog {
        id: graphConfigurationDialog
        title: "Configurar gráfico — " + window.graphOutputName
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: {
            var configurations = Object.assign({}, window.graphConfigurations)
            configurations[window.graphOutputIndex] = { minimum: graphMinimumField.text, maximum: graphMaximumField.text, resolution: graphResolutionField.text }
            window.graphConfigurations = configurations
        }
        contentItem: GridLayout {
            columns: 2
            Label { text: "Mínimo Y" }
            TextField { id: graphMinimumField; validator: DoubleValidator {} }
            Label { text: "Máximo Y" }
            TextField { id: graphMaximumField; validator: DoubleValidator {} }
            Label { text: "Resolução Y" }
            TextField { id: graphResolutionField; validator: DoubleValidator { bottom: 0.000001 } }
        }
    }
}
