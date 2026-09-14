# Ensaio de preflight do runner C com DAQC — 13.09.2026

- Ambiente: Ubuntu 22.04, ROS 2 Humble, ESP32-D0WD-V3 por CH340 em `/dev/ttyUSB0`, UART 152.000 bit/s 8N1, FMU `tests/fixtures/teste001.fmu` e perfil `tests/fixtures/esp32_profile.yaml`.
- Escopo: preflight do `fmu_rt_runner` C antes da thread FMI. Não executa malha HIL, atuação, logging, gráficos, prazo FMI ou caracterização elétrica.

## Resultado observado

Com o timeout CONFIG configurado em 100 ms, a etapa CONFIG DISABLE→ENABLE foi concluída: o preflight avançou até aguardar a confirmação `DaqcState`. A espera de estabilização ROS/XRCE foi 6.000 ms e o prazo de confirmação ROS foi ampliado somente para diagnóstico a 5.000 ms. Ainda assim, o `DaqcState` aplicado não chegou.

A instrumentação de preflight registrou 93.216 bytes RX seriais, 5.176 frames XRCE DAQC→Agent e zero datagramas Agent→DAQC, sem rejeição da ponte, coalescência ou falha serial. Uma verificação local por `pgrep` não encontrou processo `micro_ros_agent` ativo; a consulta a sockets foi limitada pelo ambiente de execução.

## Limite

O resultado identifica que o Agent local não estava ativo durante este ensaio. Ele não demonstra falha do protocolo, do firmware, do coordenador ou do controle ROS. Repetir o mesmo ensaio somente depois de iniciar o Agent pelo procedimento validado. O valor inicial de CONFIG continua 10 ms e o valor inicial da confirmação ROS continua 100 ms; os valores maiores foram usados apenas para localizar a etapa da falha.
