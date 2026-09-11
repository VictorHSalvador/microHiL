# Evidência HOST — gráficos Qt

Data: 11.09.2026. Ambiente: Ubuntu 22.04, GCC 11.4.0, CMake, Qt 6.2.4 e FMILibrary 3.0.4. Escopo: software HOST.

O teste da sessão iniciou o ciclo curto pelo caminho da GUI, encerrou-o e consumiu uma amostra da fila SPSC. A GUI foi compilada e iniciou em modo offscreen. O temporizador QML consome amostras a cada 100 ms, e cada janela recebe somente seu output, com descarte fora da janela temporal configurada.

**Resultado:** 47/47 CTests passaram; GUI compilada e iniciada offscreen.

**Limites:** não houve inspeção visual humana, DAQC, CH340, ESP32, Agent, ROS em enlace, bancada ou HIL.
