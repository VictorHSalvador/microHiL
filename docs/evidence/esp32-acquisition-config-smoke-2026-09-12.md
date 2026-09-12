# Evidência de bancada — imagem atual e CONFIG DISABLE pela CH340

Data: 12.09.2026. Ambiente: Ubuntu 22.04, ESP-IDF v5.2.6, CH340 em `/dev/ttyUSB0` (`usb-1a86_USB_Serial-if00-port0`) e ESP32-D0WD-V3 revisão 3.1.

## Procedimento e resultado observado

1. A imagem atual, produzida após regenerar a interface micro-ROS com `DaqcSetup.acquisition_frequency_hz`, foi gravada pela CH340 a 460.800 bit/s.
2. O gravador identificou ESP32-D0WD-V3, flash de 2 MB e confirmou o hash das regiões bootloader, tabela de partições e aplicação.
3. Após o reset, um cliente abriu a UART do protocolo em 152.000 bit/s, 8N1, sem RTS/CTS, e enviou exclusivamente `59 72 01 01` (CONFIG DISABLE).
4. A resposta observada foi `59 72 01 01 01`, confirmando o estado efetivo DISABLE. A placa foi deixada nesse estado.

## Limites

O resultado demonstra a gravação e o parser CONFIG seguro da imagem atual. Não houve ENABLE, STREAMING, DATA, READ_ACK, Agent, XRCE, tópico ROS, perfil aplicado, aquisição, atuação, medição temporal, bancada elétrica ou HIL.
