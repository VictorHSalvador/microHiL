# Loopback físico FMU–DAQC pelo runner C — 14.09.2026

## Objetivo e configuração

O ensaio verificou a evolução numérica do loopback analógico já descrito por `tests/fixtures/esp32_profile.yaml`: GPIO25 AO ligado fisicamente ao GPIO32 AI, `GPIO25_AO` mapeado para o output FMU `y_real`, `GPIO32_AI` mapeado para o input `u_real` e modelo `y_real = 2 × u_real`. O usuário confirmou o jumper D25→D32. Os jumpers D16→D4 e D18→D33 também permaneceram conectados, mas o perfil da fixture não mapeia o PWM e não mapeia uma entrada booleana; portanto, este ensaio não os promove a evidência de malha fechada.

O ambiente físico e as versões do Agent, firmware, ROS 2, ESP32 e CH340 são os mesmos registrados em [host-runner-daqc-fmu-2026-09-14](host-runner-daqc-fmu-2026-09-14.md). O runner usou passo de 0,01 s, duração de 2,0 s, logging binário habilitado e conversão CSV somente após o encerramento. CONFIG UART usou 100 ms neste diagnóstico; a configuração inicial do produto continua em 10 ms.

## Resultado observado

O runner completou **200 passos**, com `SCHED_FIFO` ativo, zero deadline perdido, 37.493 bytes RX, 1.200 frames TX, zero falha de I/O e término DISABLE. O logger aceitou e persistiu **200/200** amostras, sem descarte nem registro incompleto. A conversão CSV produziu 200 registros válidos de `y_real`.

Os primeiros valores de `y_real` foram 0,284; 0,284; 0,788; 0,788; 1,734; 1,730; 3,484 V. O valor ultrapassou 3,3 V no passo de sequência 6 e permaneceu com mediana aproximada de 3,482 V nos 50 registros finais. Essa evolução é compatível com a realimentação positiva: o valor aceito anteriormente é aplicado ao DAC, lido pelo ADC e dobrado pela FMU. Ao ultrapassar a faixa AO 0…3,3 V, o firmware rejeita o snapshot e mantém fisicamente o último valor aplicado, de modo que a FMU continua observando aproximadamente 1,74 V na entrada e produzindo aproximadamente 3,48 V.

O ensaio revelou que o codificador HOST aceitava um `float32` finito mesmo fora da faixa física e dependia da rejeição assíncrona do firmware. Foi criado um teste que falhou antes da correção para AO acima de 3,3 V. O codificador passou a validar, depois da transformação inversa do YAML, AO em 0…3,3 V e PWM em 0…1; antes do primeiro valor físico válido ele retorna erro de faixa e, depois dele, retém o último valor aceito. O teste cobre primeira ocorrência e retenção posterior para AO e PWM.

## Limitações

O CSV registra outputs da FMU, não a tensão medida diretamente no pino. A relação `u_real = y_real/2` decorre do modelo, mas não substitui captura simultânea do input DAQC. Não houve instrumento externo, caracterização de precisão, carga, duty intermediário, jitter ou Raspberry Pi. Os zero deadline misses em dois segundos no PC não qualificam hard real-time. A apresentação dos erros assíncronos `/daqc_errors` durante STREAMING ainda precisa ser ligada ao resultado e à GUI; esta correção reduz a geração de erros de faixa conhecidos no host, mas não substitui esse diagnóstico.
