# Ensaio físico XRCE/Agent — 11.09.2026

Ambiente: Ubuntu 22.04, ROS 2 Humble, Agent micro-ROS UDP local na porta 8888, ESP-IDF v5.2.6 e ESP32-D0WD-V3 pela CH340 em `/dev/ttyUSB0` a 152.000 bit/s.

O bridge temporário encaminhou somente MID 04 entre a UART e o Agent UDP. Antes de desabilitar o framing serial interno do Micro XRCE-DDS, a DAQC transmitiu um envelope iniciado por `7e`; o Agent o interpretou como XRCE inválido. Após a configuração `UCLIENT_PROFILE_STREAM_FRAMING=OFF`, foram observados payloads XRCE iniciados por `80` e respostas do Agent iniciadas por `81`, com chave `4d48494c`. Essa tentativa foi feita quando o leitor UART ainda iniciava após o cliente, portanto não concluiu uma sessão.

A imagem posterior, com leitor UART antes da inicialização e supervisor de retentativa, foi gravada duas vezes. CONFIG ENABLE e DISABLE continuaram respondendo como `5972010202` e `5972010101`. O bridge não recebeu MID 04 nessa imagem; a causa ainda não foi isolada. Não houve tópicos ROS, aplicação de perfil, DATA, READ_ACK de 60 s, sinais físicos, Raspberry Pi, HIL ou medição temporal.
