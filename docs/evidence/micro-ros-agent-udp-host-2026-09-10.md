# Agent micro-ROS Humble em UDP local — 10.09.2026

- Ambiente: Ubuntu 22.04; ROS 2 Humble; `micro-ROS-Agent` ramo Humble no commit `c93ee764e0d2ef4907aeb29233c68cb5f4b56976`; `micro_ros_msgs` ramo Humble; Micro-XRCE-DDS Agent v2.4.2 obtido pelo superbuild oficial.
- Escopo: build temporário do Agent e inicialização do modo UDP em loopback, sem TTY, CH340 ou DAQC.

## Procedimento executado

1. Clonar as fontes oficiais `micro-ROS-Agent` e `micro_ros_msgs` no ramo Humble.
2. Compilar ambos em workspace temporário com `colcon build` após carregar ROS 2 Humble.
3. Iniciar o executável do Agent com `udp4 -p 8888` por até dois segundos.

## Resultado observado

A primeira tentativa identificou corretamente a ausência de `micro_ros_msgs` no ambiente atual. Após incluir a dependência Humble, os dois pacotes compilaram com sucesso. O Agent iniciou e informou escuta UDP na porta 8888.

## Limites

Não foi criada sessão XRCE, não houve troca de mensagens ROS, execução da ponte junto ao Agent, acesso à CH340, ESP32, DAQC, FMU, bancada, HIL ou medição de tempo real. O resultado comprova somente a disponibilidade local do processo UDP do Agent no ambiente temporário.
