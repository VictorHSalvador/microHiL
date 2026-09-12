# Evidência HOST — controle de gráficos durante execução

Data: 12.09.2026. Ambiente: Ubuntu 22.04, GCC 11.4.0, CMake, Qt 6.2.4 e FMILibrary externa do build `/tmp/microhil-gui-fix`. Escopo: software HOST.

O teste `execution_session_lifecycle` iniciou uma execução com publicação gráfica desabilitada e confirmou que a fila não recebeu amostras. Em uma segunda execução, ele habilitou a publicação após o início e confirmou o recebimento de uma amostra, mantendo a conclusão de quatro passos e o log binário exportável.

**Resultado observado:** compilação concluída e 47/47 CTests aprovados; o teste XRCE de loopback foi pulado por pré-condição de ambiente.

**Limites:** não houve inspeção visual manual, medição de carga da GUI, FMU de produto, DAQC, ESP32, CH340, ROS, Raspberry Pi, bancada ou HIL.
