# Revalidação física do loopback após proteção de faixa — 14.09.2026

## Objetivo e ambiente

O ensaio repetiu a malha FMU–DAQC descrita em [physical-fmu-loopback-2026-09-14](physical-fmu-loopback-2026-09-14.md) depois da validação HOST de AO em 0…3,3 V e PWM em 0…1. O ESP32-D0WD-V3 permaneceu ligado pela CH340 em `/dev/ttyUSB0`, a 152.000 bit/s 8N1, com o jumper GPIO25 AO→GPIO32 AI. O runner C foi o único proprietário da TTY e encaminhou XRCE ao Micro-ROS Agent UDP local. Um assinante ROS 2 foi iniciado com o tipo explícito `microhil_interfaces/msg/DaqcErrors` antes da execução.

O runner usou `tests/fixtures/teste001.fmu`, `tests/fixtures/esp32_profile.yaml`, passo de 0,01 s, duração de 2,0 s, logging binário e conversão CSV posterior. O executável possuía `cap_sys_nice=ep`.

## Resultado observado

O processo encerrou com código zero e estado `FINISHED`. Foram concluídos **200 passos**, com `SCHED_FIFO` ativo, zero deadline perdido, zero liberação não utilizada, pior tempo de cálculo FMU de **0,040763 ms**, 14.739 bytes RX, 677 frames TX e zero falha de I/O. O logger persistiu **200/200** amostras e o CSV contém 200 registros.

`y_real` iniciou em 0,284 V, passou por aproximadamente 0,800 V e 1,766 V e permaneceu próximo de 3,48 V no fim. Esse resultado reproduz a evolução da realimentação física após a proteção HOST de faixa. Nenhuma mensagem foi observada em `/daqc_errors` durante o intervalo em que o assinante tipado permaneceu ativo.

## Limitações

A ausência de mensagem em `/daqc_errors` significa apenas que o tópico não publicou erro observado pelo assinante durante este ensaio; não demonstra ausência de toda falha interna do firmware. O CSV contém outputs FMU, sem captura simultânea do input DAQC por canal. Não houve instrumento externo, caracterização elétrica, carga, duty PWM intermediário, execução prolongada, Raspberry Pi ou qualificação de hard real-time.
