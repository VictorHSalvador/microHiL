# Evidência HOST — configuração de aquisição DAQC

Data: 12.09.2026.

## Escopo executado

1. O pacote ROS 2 Humble `microhil_interfaces` foi gerado em diretório temporário com o novo campo `DaqcSetup.acquisition_frequency_hz`.
2. O runner com controle ROS foi compilado contra essas interfaces geradas.
3. O CTest dessa variante concluiu 48 testes, incluindo publicação tipada de `DaqcSetup`; o ensaio XRCE de loopback foi ignorado pelo ambiente, como configurado.
4. A variante Qt/HOST foi recompilada e concluiu 47 testes; o carregador YAML validou `acquisition.frequency_hz=100` na fixture.

## Limites

A revisão posterior 0.26.1 limitou a taxa a 100 Hz após inspecionar `CONFIG_FREERTOS_HZ=100`; esta evidência de build antecede essa correção. Não houve build ESP-IDF desta alteração porque o SDK previamente usado em `/tmp/esp-idf-v5.2.6` não está disponível após a reinicialização do ambiente. Não houve gravação, CH340, ESP32, Agent, sessão XRCE, DATA físico, medição de frequência/jitter, bancada ou HIL. A fonte do firmware foi alterada, mas esses resultados só verificam o contrato e o código HOST.
