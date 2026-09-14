# Ensaio de preflight do runner C com DAQC — 13.09.2026

- Ambiente: Ubuntu 22.04, ROS 2 Humble, ESP32-D0WD-V3 por CH340 em `/dev/ttyUSB0`, UART 152.000 bit/s 8N1, FMU `tests/fixtures/teste001.fmu` e perfil `tests/fixtures/esp32_profile.yaml`.
- Escopo: preflight do `fmu_rt_runner` C antes da thread FMI. Não executa malha HIL, atuação, logging, gráficos, prazo FMI ou caracterização elétrica.

## Resultado observado

Com o timeout CONFIG configurado em 100 ms, a etapa CONFIG DISABLE→ENABLE foi concluída: o preflight avançou até aguardar a confirmação `DaqcState`. A espera de estabilização ROS/XRCE foi 6.000 ms e o prazo de confirmação ROS foi ampliado somente para diagnóstico a 5.000 ms. Ainda assim, o `DaqcState` aplicado não chegou.

A instrumentação de preflight registrou 93.184 bytes RX seriais, 5.174 frames XRCE DAQC→Agent e zero datagramas Agent→DAQC, sem rejeição da ponte, coalescência ou falha serial. Depois do ensaio, foi confirmado o Agent correto em UDP 8888 (PID 22691) e o harness Python repetiu o ciclo completo com essa mesma instância: ENABLE, setup, STREAMING, 70 DATA em 1,01 s e DISABLE. Portanto, a ausência de resposta é limitada à ponte C atual, não à disponibilidade do Agent ou da DAQC.

## Limite

O resultado não identifica a causa interna da incompatibilidade da ponte C. A comparação demonstra somente que o caminho Python conhecido recebe respostas do Agent, enquanto o caminho C atual não as recebe. Não alterar o protocolo, a fila XRCE ou os prazos com base apenas neste resultado. O valor inicial de CONFIG continua 10 ms e o valor inicial da confirmação ROS continua 100 ms; os valores maiores foram usados apenas para localizar a etapa da falha.
