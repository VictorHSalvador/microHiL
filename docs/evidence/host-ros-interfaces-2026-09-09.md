# Evidência HOST — interfaces ROS 2 da DAQC

Data: 09.09.2026  
Ambiente: Ubuntu 22.04.5 x86-64; ROS 2 Humble instalado em `/opt/ros/humble`; `colcon`; build e instalação temporários em `/tmp/microhil-ros-build` e `/tmp/microhil-ros-install`.

## Procedimento executado

1. Criar o pacote `ros2/microhil_interfaces` como `ament_cmake`.
2. Declarar `DaqcSetup`, `DaqcState` e `DaqcErrors` conforme IF-ROS.
3. Executar `colcon build --base-paths ros2 --packages-select microhil_interfaces` com diretórios temporários.
4. Carregar a instalação temporária e consultar cada interface com `ros2 interface show`.

## Resultado observado

O pacote compilou sem erro. As três interfaces geradas contêm os campos, os códigos DISABLE/ENABLE/STREAMING e as flags binárias definidos pelo ICD.

## Limites

Não há firmware, cliente micro-ROS, Agent integrado, tópico em execução, CH340, ESP32 ou comunicação ROS 2 medida nesta evidência. `Esp32Data` não foi criado: sua estrutura depende dos I/Os simultâneos e nomes concretos do perfil ESP32, ainda não definidos.
