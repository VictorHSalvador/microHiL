# Ensaio físico STREAMING, aquisição e confirmação de leitura — 13.09.2026

## Procedimento e condições de ensaio

Data: 13.09.2026
Ambiente: Host Ubuntu 22.04.5 LTS, ROS 2 Humble, ESP32-D0WD-V3 (rev v3.1) conectado via CH340 em `/dev/ttyUSB0` a 152.000 bps 8N1. O firmware gravado na placa é a imagem 0.26.7 (`firmware/daqc_esp32/build-xrce-0267/microhil_daqc_esp32.bin`). O micro-ROS Agent executou na porta UDP local 8888. A ferramenta executada foi `tools/esp32_streaming_smoke.py`.

## Resultados observados

1. **Rejeição de segurança sem configuração (REQ-F-02 / REQ-F-18):**
   Ao solicitar `CONFIG STREAMING` (`59 72 01 03`) com a DAQC em `ENABLE` sem aplicar o perfil físico via ROS 2, a DAQC recusou a transição e permaneceu em `state=2` (ENABLE). Esse comportamento atende à especificação, impedindo a ativação de I/O não configurado.

2. **Ciclo integrado com configuração e STREAMING:**
   - **DISABLE → ENABLE:** comando `59 72 01 02` confirmado com resposta `59 72 01 02 02`.
   - **Configuração ROS:** publicação de `microhil_interfaces/msg/DaqcSetup` com `command=2`, `profile_id=1`, `apply_configuration=1`, ADC 12 bits / 11 dB, PWM 1000 Hz / 10 bits e aquisição a 100 Hz. O firmware aplicou `DaqcProfileConfigure` com sucesso.
   - **ENABLE → STREAMING:** comando `59 72 01 03` aceito pela DAQC com confirmação `59 72 01 03 03`.
   - **Aquisição periódica (REQ-F-19 / REQ-NF-16 / REQ-NF-17):** o Núcleo 1 (`IoTask`) transmitiu continuamente frames `DATA` (MID 02) de 33 bytes. Foram coletados 289 frames em 3,00 segundos, correspondendo a uma frequência observada de **~96,4 Hz** (nominal: 100 Hz). Os frames continham as 4 entradas digitais (`DI=[0, 0, 1, 0]`) e os 6 canais analógicos calibrados em volts (`AI=[0.635, 0.405, 0.142, 0.142, 0.142, 0.142] V`).
   - **Confirmação de leitura (REQ-F-27):** para cada frame `DATA` recebido, o host transmitiu `READ_ACK` (MID 03) com o número de sequência unívoco, comprovando consumo pelo host e renovando o watchdog.
   - **Encerramento seguro (REQ-F-07):** envio de `CONFIG DISABLE` (`59 72 01 01`) resultou em confirmação imediata `59 72 01 01 01` e cessou o streaming. O firmware requisita o nível seguro nas saídas nesse estado; este ensaio não mediu eletricamente os pinos para confirmar tensão ou duty cycle nulos.

## Limitações

O ensaio foi executado com o harness Python sobre a UART física e ponte UDP. A atuação de dados host→DAQC de 21 bytes não foi exercitada neste ensaio. Os canais analógicos e digitais leram sinais de pinos flutuantes/pulls internos; os valores em Volts foram produzidos pela calibração por line fitting do ESP-IDF, sem instrumento de bancada para validar sua precisão elétrica. A integração completa com o coordenador C (`fmu_rt_runner`) permanece como próximo passo.
