# Evidência HOST — extensão de configuração ROS da DAQC

Data: 10.09.2026  
Ambiente: Ubuntu 22.04.5 x86-64; ROS 2 Humble; workspace limpo em `/tmp/microhil-ros-interfaces`.

## Procedimento executado

1. Criar workspace limpo contendo o pacote `microhil_interfaces`.
2. Compilar somente esse pacote com `colcon build --packages-select microhil_interfaces` após carregar o ambiente ROS 2 Humble.
3. Inspecionar as interfaces geradas `DaqcSetup`, `DaqcState` e `DaqcErrors` com `ros2 interface show`.

## Resultado observado

O pacote foi compilado com sucesso. A inspeção mostrou em `DaqcSetup` os campos de aplicação, resolução/atenuação ADC e frequência/resolução PWM; em `DaqcState`, `configuration_applied`; e em `DaqcErrors`, as flags binárias de configuração ADC e PWM.

## Limites

Essa evidência não compila firmware ESP-IDF, não inicializa micro-ROS, Agent, UART, CH340 ou ESP32 e não transmite mensagens. Ela prova somente a geração e a visibilidade das interfaces ROS 2 no HOST.
