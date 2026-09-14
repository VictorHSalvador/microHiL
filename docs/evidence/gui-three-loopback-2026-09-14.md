# Ensaio físico dos três loopbacks pela GUI — 14.09.2026

## Objetivo e ambiente

O ensaio verificou o Play HiL da GUI Qt com a FMU `tests/fixtures/MicroHiL_LoopbackTest.fmu` e o perfil `tests/fixtures/microhil_loopback_profile.yaml`. A FMU FMI 2.0 Co-Simulation possui três comandos físicos: `do_command` para GPIO16 DO, `ao_command_v` para GPIO25 AO e `pwm_command` para GPIO18 PWM. Os retornos são `di_feedback` por GPIO4 DI, `ao_feedback` por GPIO32 AI e `pwm_feedback` por GPIO33 AI. Os jumpers físicos informados foram D16→D4, D25→D32 e D18→D33. Os outputs `do_feedback_graph`, `ao_feedback_graph` e `pwm_feedback_graph` ficaram fora do mapa DAQC e foram registrados somente para verificação.

O ambiente foi PC Ubuntu 22.04, ROS 2 Humble, Micro-ROS Agent UDP local na porta 8888, ESP32-D0WD-V3 pela CH340 em `/dev/ttyUSB0`, UART 152.000 bit/s 8N1 e firmware 0.26.7 já gravado. A GUI recebeu `CAP_SYS_NICE` e executou passo de 0,01 s por 5 s.

## Resultado observado

A interface apresentou `Finished — complete`, `simulation completed`, 500 passos concluídos, zero deadlines perdidos, zero liberações não usadas, pior computação de 0,061 ms e pior atraso de 0,000 ms. A estatística do enlace indicou 23.130 bytes RX, 1.196 frames TX, 1.615 leituras sem bytes disponíveis no polling e zero falhas de I/O. O log binário foi fechado com 500 registros.

A leitura do log fechado confirmou os intervalos abaixo:

| Sinal | Intervalo observado | Interpretação limitada |
|---|---:|---|
| `ao_command_v` | 0,650…2,650 V | Comando DAC permaneceu na faixa física definida pela FMU. |
| `ao_feedback_graph` | 0,142…2,597 V | A aquisição retornou valores variáveis coerentes com o jumper GPIO25→GPIO32, sem declarar precisão. |
| `do_command` | 0…1 | Pulso digital foi emitido. |
| `do_feedback_graph` | 0…1 | Retorno digital assumiu ambos os estados no jumper GPIO16→GPIO4. |
| `pwm_command` | 0,203…0,797 | Duty transmitido permaneceu na faixa normalizada. |
| `pwm_feedback_graph` | 0,142…3,118 V | ADC observou o sinal PWM direto no jumper GPIO18→GPIO33. |

## Limites

O jumper PWM direto não filtra a portadora de 20 kHz antes do ADC; por isso a leitura não mede duty intermediário nem permite comparar quantitativamente `pwm_command` e `pwm_feedback_graph`. Um filtro RC e medição adequada continuam necessários para essa qualificação. Os 1.615 timeouts são leituras sem bytes no polling limitado de 5 ms, não falhas de I/O ou retransmissões. O ensaio não caracteriza precisão ADC/DAC, carga elétrica, duração longa, Raspberry Pi, jitter ou hard real-time.
