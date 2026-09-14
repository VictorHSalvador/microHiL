# Ensaio de preflight do runner C com DAQC — 13.09.2026

- Ambiente: Ubuntu 22.04, ROS 2 Humble, ESP32-D0WD-V3 por CH340 em `/dev/ttyUSB0`, UART 152.000 bit/s 8N1, FMU `tests/fixtures/teste001.fmu` e perfil `tests/fixtures/esp32_profile.yaml`.
- Escopo: preflight do `fmu_rt_runner` C antes da thread FMI. Não executa malha HIL, atuação, logging, gráficos, prazo FMI ou caracterização elétrica.

## Resultado observado

Com o timeout CONFIG configurado em 100 ms, a etapa CONFIG DISABLE→ENABLE foi concluída: o preflight avançou até aguardar a confirmação `DaqcState`. A espera de estabilização ROS/XRCE foi 6.000 ms e o prazo de confirmação ROS foi ampliado somente para diagnóstico a 5.000 ms. Ainda assim, o `DaqcState` aplicado não chegou.

A instrumentação de preflight registrou 93.184 bytes RX seriais, 5.174 frames XRCE DAQC→Agent e zero datagramas Agent→DAQC, sem rejeição da ponte, coalescência ou falha serial. Depois do ensaio, foi confirmado o Agent correto em UDP 8888 (PID 22691) e o harness Python repetiu o ciclo completo com essa mesma instância: ENABLE, setup, STREAMING, 70 DATA em 1,01 s e DISABLE. Portanto, a ausência de resposta é limitada à ponte C atual, não à disponibilidade do Agent ou da DAQC.

Uma variante temporária do runner registrou como primeiro payload DAQC→Agent da ponte C `81 00 00 00 0b 01 05 00 0d 00 0d 00 80`. Em contraste, o traço do harness funcional mostrou que seu primeiro payload DAQC→Agent foi `80 00 00 00 00 01 10 00 58 52 43 45 01 00 01 0f 4d 48 49 4c 81 00 7c 00`, seguido de uma resposta do Agent iniciada por `81`. A diferença de ponto de entrada é observada; a hipótese de bytes UART anteriores não é confirmação de causa.

O runner foi então alterado para reproduzir o baseline do harness: após abrir a TTY, ele descarta RX pendente e exige uma nova confirmação CONFIG DISABLE antes de ENABLE. O ensaio físico confirmou que esse baseline avançou, mas ainda falhou em `DaqcState configuration confirmation`, com 93.664 bytes RX, 5.199 frames DAQC→Agent e zero Agent→DAQC. Assim, bytes pendentes não explicam sozinhos a incompatibilidade.

## Limite

O resultado não identifica a causa interna da incompatibilidade da ponte C. A comparação demonstra somente que o caminho Python conhecido recebe respostas do Agent, enquanto o caminho C atual não as recebe. Não alterar o protocolo, a fila XRCE ou os prazos com base apenas neste resultado. O valor inicial de CONFIG continua 10 ms e o valor inicial da confirmação ROS continua 100 ms; os valores maiores foram usados apenas para localizar a etapa da falha.
