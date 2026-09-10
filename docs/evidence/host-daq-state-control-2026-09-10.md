# Evidência — transições CONFIG da DAQC em HOST

- Data: 10.09.2026.
- Ambiente: Ubuntu 22.04; build CMake com FMILibrary fixada; sem dispositivo físico.
- Procedimento: build em `/tmp/microhil-runner-check` e execução de `ctest --test-dir /tmp/microhil-runner-check --output-on-failure`.
- Resultado observado: 45 de 45 testes passaram. `daq_state_control_transitions` confirmou ENABLE→STREAMING e STREAMING→DISABLE após quadros CONFIG de confirmação injetados, rejeitou Play fora de ENABLE e reportou timeout quando não recebeu confirmação.
- Escopo verificado: prazo de controle fora da thread FMI, repetição no máximo a cada 5 ms e API configurável de timeout; `AppConfig` inicia em 10 ms.
- Limitações: o teste usa coordenador e frames em memória. Não exercita TTY, CH340, ESP32, temporização física, Agent, FMU em execução, GPIO, tensão, DAC, watchdog ou HIL.
