# Evidência HOST — limite de aquisição pelo tick FreeRTOS

Data: 12.09.2026.

## Procedimento e resultado

1. A inspeção de `firmware/daqc_esp32/sdkconfig` identificou `CONFIG_FREERTOS_HZ=100`.
2. A espera da tarefa de I/O foi alterada para um tick explícito, eliminando a conversão de 1 ms que produzia zero ticks nesse `sdkconfig`.
3. O carregador YAML passou para o limite 1…100 Hz; as fixtures cobrem perfil legado sem a chave, 100 Hz explícitos e rejeição de 101 Hz.
4. A build Qt/HOST e os 47 CTests concluíram sem falhas; o teste documental concluiu 29 verificações.

## Limites

Não houve build ESP-IDF, gravação, CH340, ESP32, Agent, XRCE, aquisição física, medição de frequência/jitter, bancada ou HIL. O `sdkconfig` inspecionado é artefato de build anterior; a aplicação final da correção no alvo continua pendente.
