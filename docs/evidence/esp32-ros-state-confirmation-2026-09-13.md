# Observação física de confirmação ROS da DAQC — 13.09.2026

- Ambiente: Ubuntu 22.04 com ROS 2 Humble, ESP32-D0WD-V3 ligado por CH340 em `/dev/ttyUSB0`, UART 152.000 bit/s 8N1 e Agent micro-ROS UDP local na porta 8888.
- Escopo: observar a confirmação ROS de configuração pelo firmware durante o procedimento físico já usado pelo harness. Não executa o coordenador C, uma FMU, atuação, malha HIL, medição de prazo ou qualificação temporal.

## Procedimento executado

Em um terminal, o Agent foi iniciado com:

```bash
source /opt/ros/humble/setup.bash
source /tmp/microhil-agent-ws/install/setup.bash
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888
```

Em outro terminal, foi usado o procedimento conhecido:

```bash
cd ~/Projects/microHiL
python3 tools/esp32_streaming_smoke.py --port /dev/ttyUSB0 --auto-publish-setup --streaming-duration 5.0 --no-reset
```

O harness confirmou DISABLE, ENABLE, publicou `DaqcSetup` do perfil 1, entrou em STREAMING, coletou 478 frames DATA em 4,98 s (~96,1 Hz), enviou READ_ACK e confirmou o DISABLE de encerramento.

Separadamente, um assinante passivo em `/daqc_state` observou primeiro `state=2`, `profile_id=1`, `profile_applied=0` e `configuration_applied=0`; após a publicação do setup, observou `state=2`, `profile_id=1`, `profile_applied=1` e `configuration_applied=1`.

## Resultado e limite

A DAQC confirmou pelo ROS a aplicação do perfil e da configuração durante o procedimento físico. A espera de estabilização de 6.000 ms usada pelo harness foi incorporada como parâmetro prévio à publicação de `DaqcSetup`; ela não é uma confirmação de configuração. A confirmação continua sendo o `DaqcState` aplicado dentro do prazo ROS configurável. Esta evidência não demonstra que o `fmu_rt_runner` C, o coordenador, a FMU ou a malha fechada operaram com a placa.
