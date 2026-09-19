# EV-MICRO-ROS-AGENT-PERSISTENTE-2026-09-19

Data de execução: 19.09.2026. Ambiente: Ubuntu 22.04 com ROS 2 Humble instalado em `/opt/ros/humble`.

## Procedimento executado

Após o reinício que removeu o workspace anterior em `/tmp/microhil-agent-ws`, foram clonados os ramos `humble` de `micro-ROS-Agent` e `micro_ros_msgs` em `.local/microhil-agent-ws/src/`. Com o ambiente ROS 2 Humble carregado, foi executado:

```bash
colcon build --packages-select micro_ros_msgs micro_ros_agent
```

Os commits observados foram `c93ee764e0d2ef4907aeb29233c68cb5f4b56976` para `micro-ROS-Agent` e `c9062eb3860d16c1bff1423923de3b0956fd4734` para `micro_ros_msgs`. O comando abaixo iniciou o Agent UDP por três segundos e confirmou a escuta na porta 8888:

```bash
source /opt/ros/humble/setup.bash
source /home/linuxvh/Projects/microHiL/.local/microhil-agent-ws/install/setup.bash
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888 -v6
```

## Resultado observado

O executável `micro_ros_agent` foi localizado no prefixo instalado e registrou `running... | port: 8888`. O workspace passa a ficar em diretório persistente, ignorado pelo Git.

## Limites

O ensaio não abriu a CH340, não recebeu XRCE do ESP32, não publicou tópicos ROS, não enviou CONFIG/DATA/READ_ACK e não executou FMU, GUI ou HIL. Ele comprova somente a reconstrução e a escuta local do Agent no ambiente atual.
