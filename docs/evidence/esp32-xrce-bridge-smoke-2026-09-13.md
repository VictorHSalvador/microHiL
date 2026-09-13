# Ensaio da ponte MID 04 da DAQC — 13.09.2026

## Procedimento e resultado

A imagem 0.26.7 permaneceu gravada. A CH340 foi aberta antes de ajustar DTR/RTS, conforme o procedimento manual que confirmou CONFIG. Com a DAQC em DISABLE e o Agent micro-ROS Humble UDP local na porta 8888, uma ponte temporária foi executada por 8 s.

CONFIG foi confirmado. Foram observados 468 frames MID 04 DAQC→Agent e 285 frames Agent→DAQC. `ros2 node list` mostrou `/microhil_daqc`. `ros2 topic list` mostrou `/daqc_errors`, `/daqc_setup`, `/daqc_state`, `/parameter_events` e `/rosout`.

## Limites

O ensaio ocorreu inteiramente em DISABLE. Não houve STREAMING, DATA, aquisição, atuação, I/O físico, medição de timing nem HIL. Os contadores de frames não demonstram integridade de mensagens, QoS, latência ou comportamento sob carga.
