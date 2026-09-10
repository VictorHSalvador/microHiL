# Controle ROS 2 `rcl` no runner HOST — 10.09.2026

- Ambiente: Ubuntu 22.04; ROS 2 Humble; interfaces `microhil_interfaces` compiladas localmente; GCC 11.4; CMake.
- Escopo: componente `DaqcRosControl`, thread `rcl`, publicação tipada de `DaqcSetup` e retenção de `DaqcState`/`DaqcErrors`.

## Procedimento executado

1. Compilar `microhil_interfaces` em instalação temporária com `colcon` após carregar ROS 2 Humble.
2. Configurar o runner com `MICROHIL_WITH_ROS2_CONTROL=ON` e a instalação temporária das interfaces.
3. Iniciar a thread `rcl` no teste, publicar um `DaqcSetup` do perfil ESP32 e injetar estado/erro tipados no componente.
4. Executar CTest da variante ROS.

## Resultado observado

O componente iniciou, publicou a mensagem de setup e classificou corretamente uma confirmação ENABLE do perfil 1 e um erro de perfil. A variante ROS concluiu **47/47** testes.

## Limites

Não houve Agent, sessão XRCE, ponte UDP ativa junto ao Agent, CH340, ESP32, DAQC, tópico remoto, CONFIG físico, FMU em execução, bancada, HIL ou medição de interferência/tempo real. O prazo de espera por `DaqcState` permanece pendente de decisão antes de ligar o componente ao Play.
