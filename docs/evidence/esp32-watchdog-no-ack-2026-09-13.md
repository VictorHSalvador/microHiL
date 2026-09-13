# Ensaio físico do watchdog sem READ_ACK — 13.09.2026

## Procedimento e ambiente

Host Ubuntu 22.04.5 LTS com ROS 2 Humble; ESP32-D0WD-V3 pela CH340 em `/dev/ttyUSB0`, 152.000 bit/s 8N1. A placa continha a imagem 0.26.7. O micro-ROS Agent Humble executou em UDP local na porta 8888. Foi usado `tools/esp32_streaming_smoke.py --auto-publish-setup --watchdog-no-ack-duration 65 --setup-wait 8`.

O harness confirmou DISABLE, entrou em ENABLE, publicou `DaqcSetup` do perfil 1 e confirmou STREAMING. Durante os 65 s de coleta, o host leu os frames DATA, mas não transmitiu nenhum READ_ACK. O bloco `finally` solicitou CONFIG DISABLE ao encerrar o ensaio.

## Resultado observado

Foram observados 5.983 frames DATA. O primeiro e o último frame abrangem 59,98 s, em frequência calculada de aproximadamente 99,7 Hz. O último DATA foi recebido a 60,00 s após a supressão de READ_ACK. Não houve DATA durante os 5 s restantes de observação. O cleanup recebeu confirmação `state=1` para DISABLE.

O resultado é compatível com a transição autônoma para DISABLE exigida por REQ-F-27 ao completar 60 s contínuos sem avanço da confirmação cumulativa. A tolerância de supervisão observada, pela resolução deste harness, foi inferior ao intervalo de impressão/observação após o marco de 60 s; não foi medida com instrumento temporal externo.

## Limitações

O ensaio demonstra o comportamento do firmware e do enlace serial desta placa. Não mediu tensão, duty cycle ou carga nas saídas; não enviou DATA host→DAQC, não executou o coordenador C e não realizou uma simulação HIL.
