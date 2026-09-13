# Ensaio físico de atuação DATA com loopbacks ESP32 — 13.09.2026

## Procedimento e ambiente

Host Ubuntu 22.04.5 LTS com ROS 2 Humble; ESP32-D0WD-V3 (rev. v3.1) conectado pela CH340 em `/dev/ttyUSB0`, a 152.000 bit/s 8N1. A imagem foi recompilada com ESP-IDF v5.2.6 e gravada no dispositivo. O micro-ROS Agent Humble executou em UDP local na porta 8888.

Foram instalados três jumpers diretos na placa: GPIO16 (DO) para GPIO4 (DI), GPIO25 (AO DAC) para GPIO32 (AI) e GPIO18 (PWM) para GPIO33 (AI). Foi executado `python3 tools/esp32_actuation_loopback.py --setup-wait 8 --sample-duration 1`, que confirma DISABLE, entra em ENABLE, publica `DaqcSetup` do perfil 1, entra em STREAMING, encaminha XRCE e confirma cada DATA DAQC→host com READ_ACK. O ensaio envia um frame DATA host→DAQC de 21 bytes com GPIO16=1, AO GPIO25=1,65 V e PWM GPIO18 com duty 1,0.

## Resultado observado

O ensaio concluiu com sucesso e confirmou DISABLE no cleanup. A aquisição observou GPIO4 alto em 95 de 96 amostras; o valor mediano de GPIO32 passou de 0,142 V para 1,6715 V; o valor mediano de GPIO33 passou de 0,142 V para 3,118 V. Isso demonstra, nesta placa e com estes jumpers, o encaminhamento de DATA host→DAQC, aplicação no núcleo 1 e retorno pelos canais DI/AI para os recursos DO GPIO16, DAC GPIO25 e PWM GPIO18 em nível alto.

Durante o diagnóstico, o primeiro arranjo aplicava GPIO/DAC/PWM diretamente na tarefa de comunicação do núcleo 0. A versão ensaiada passa apenas um snapshot limitado da comunicação para a tarefa de I/O no núcleo 1; um frame novo substitui o snapshot ainda não aplicado e um frame inválido não deixa o parser preso. A aplicação de DAC é tentada mesmo se uma atualização PWM falhar, evitando que uma falha de um periférico oculte o resultado do outro.

## Limitações

O GPIO33 não é instrumento de caracterização de PWM. Um duty de 0,5 a 937 Hz não produziu uma mediana representativa na aquisição a 100 Hz, pois a aquisição não inclui filtro passa-baixa nem mede duty cycle. Por isso o ensaio de routing usa duty 1,0, que cria nível alto estático verificável por AI. A validação de duty intermediário, frequência, bordas, ripple, carga e tensão precisa de osciloscópio ou instrumento equivalente e de procedimento de bancada próprio.

O ensaio usa o harness Python e a ponte UDP, não o coordenador C com `fmu_rt_runner`, não executa uma FMU em malha fechada e não qualifica deadline, jitter, precisão elétrica, carga ou Raspberry Pi.
