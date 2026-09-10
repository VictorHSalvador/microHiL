# Evidência — transporte UART e watchdog do firmware ESP32 em HOST

- Data: 10.09.2026.
- Ambiente: Ubuntu 22.04; ESP-IDF v5.2.6; componente `micro_ros_espidf_component` no branch Humble; alvo de compilação `esp32`.
- Procedimento: centralização da leitura e transmissão UART em `daqc_transport`; recompilação cruzada com `. /tmp/esp-idf-v5.2.6/export.sh && idf.py build` em `firmware/daqc_esp32`.
- Resultado observado: o binário `microhil_daqc_esp32.bin` foi gerado com tamanho `0x2dca0`; a menor partição de aplicação tem `0x100000`, com `0xd2360` livres.
- Escopo verificado: compilação do caminho único de TX protegido por mutex, envio CONFIG/DATA/XRCE pelo transporte, leitura UART centralizada e renovação do watchdog de 60 s somente no primeiro ou em `READ_ACK` modularmente mais novo.
- Limitações: não houve gravação na placa, CH340, transmissão UART, confirmação de estado, medição de 60 s, concorrência observada, Agent micro-ROS, tópicos ROS, tensão, calibração, deadline ou HiL. A compilação não prova que o mutex, a cadência ou o watchdog atendem ao comportamento temporal no alvo.
