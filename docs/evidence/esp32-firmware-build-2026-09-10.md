# Evidência — build do firmware ESP32 em HOST

- Data: 10.09.2026
- Ambiente: Ubuntu 22.04, ESP-IDF v5.2.6 e componente `micro_ros_espidf_component` no branch Humble.
- Procedimento: instalação das dependências Python exigidas pelo componente e execução de `idf.py build` em `firmware/daqc_esp32`.
- Resultado observado: build do firmware e geração de `microhil_daqc_esp32.bin` concluídos. O binário ocupou `0x2db90` de uma partição de `0x100000` bytes, com 82% livres.
- Escopo verificado: compilação cruzada do parser, transporte UART, integração da biblioteca micro-ROS, leitura ADC calibrada por driver, GPIO, DAC e PWM do perfil ESP32.
- Limitações: a placa não foi gravada nem conectada. Esta evidência não valida tensão, calibração efetiva, mapeamento elétrico, comunicação CH340, Agent XRCE, tópicos ROS, timeout de 60 s, concorrência em execução, deadlines ou HiL.
