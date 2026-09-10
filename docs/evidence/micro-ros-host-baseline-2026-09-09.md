# EV-MICRO-ROS-HOST-BASELINE-2026-09-09

Data de execução: 09.09.2026. Ambiente observado: Ubuntu 22.04.5, ROS 2 Humble já instalado, `colcon` e `rosdep` disponíveis. Diretório temporário: `/tmp/microhil-microros-baseline`.

## Procedimento e resultado observado

Foram clonados de forma limpa os ramos `humble` de `micro_ros_setup`, `micro_ros_msgs`, `micro-ROS-Agent` e `micro_ros_espidf_component`. Após carregar `/opt/ros/humble/setup.sh`, foi executado `colcon build --packages-select micro_ros_msgs micro_ros_agent`.

O build concluiu `micro_ros_msgs` e `micro_ros_agent` com sucesso. A primeira tentativa, sem `micro_ros_msgs`, falhou por dependência ausente; a repetição depois de obter a dependência foi bem-sucedida. O Agent baixou e compilou o Micro XRCE-DDS Agent como dependência transitiva.

| Item | Commit observado | Estado desta evidência |
|---|---|---|
| micro_ros_setup, ramo humble | `af209288676e5f02ac7c6d419b8ad157d3bed14e` | Clonado; package buildou no host |
| micro_ros_msgs, ramo humble | `c9062eb3860d16c1bff1423923de3b0956fd4734` | Build HOST concluído |
| micro-ROS-Agent, ramo humble | `c93ee764e0d2ef4907aeb29233c68cb5f4b56976` | Build HOST concluído |
| Micro XRCE-DDS Agent transitivo | `57d086216d01ec43121845d385894a25987f8a2c` | Compilado como dependência do Agent |
| micro_ros_espidf_component, ramo humble | `4ddd8c26e721662319ed8af981cb7cdc9ae05382` | Clonado apenas; ESP-IDF não estava instalado |

## Limitação e consequência

O README do componente ESP-IDF clonado lista ESP-IDF 5.2, 5.3, 5.4, 5.5 e 6.0 como versões testadas. Esta execução não verificou compatibilidade com ESP-IDF 4.4.8, não compilou firmware, não comunicou pela CH340, não executou transporte customizado, nem validou ROS/DDS, placa, temporização, sinais elétricos ou HIL.

A diferença entre a declaração do componente e DEC-006 é uma pendência de decisão de produto. Esta evidência não autoriza troca de SDK nem fixa o componente como baseline definitiva.
