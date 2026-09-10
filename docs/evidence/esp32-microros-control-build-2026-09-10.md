# Evidência — controle micro-ROS da DAQC em HOST

- Data: 10.09.2026.
- Ambiente: Ubuntu 22.04; ESP-IDF v5.2.6; componente `micro_ros_espidf_component` e interfaces `microhil_interfaces` no ramo Humble; alvo de compilação `esp32`.
- Procedimento: implementação do controlador de estado e do nó micro-ROS com transporte customizado MID 04; execução de `. /tmp/esp-idf-v5.2.6/export.sh && idf.py build` em `firmware/daqc_esp32`.
- Resultado observado: build cruzado concluído sem avisos do compilador; `microhil_daqc_esp32.bin` tem `0x43d30` bytes e a partição de aplicação de `0x100000` deixa `0xbc2d0` bytes livres.
- Escopo verificado: compilação das mensagens `DaqcSetup`, `DaqcState` e `DaqcErrors`, transporte customizado XRCE, nó `microhil_daqc`, assinante `/daqc_setup`, publicadores `/daqc_state` e `/daqc_errors`, e separação de CONFIG como autoridade de estado.
- Limitações: sem gravação na placa, Agent integrado, UART CH340, tópicos observados, configuração ADC/PWM aplicada, dados DATA, watchdog de 60 s, medição de stack/prioridade, deadlines, tensão, calibração ou HiL. O build não prova interoperabilidade ROS, prioridade efetiva ou comportamento no alvo.
