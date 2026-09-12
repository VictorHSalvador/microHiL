# Correções da GUI após inspeção manual — 12.09.2026

O operador observou que fechar um gráfico e executar novamente gerava acesso QML a `graph.samples` indefinido na linha 432 de `Main.qml`; o Play debug também tentava configurar `SCHED_FIFO` e imprimia avisos em toda execução.

A janela de gráfico agora sinaliza seu descarte, remove a própria entrada do registro de gráficos e o consumidor confirma que `samples` é um array antes de copiá-lo. Entradas Boolean virtuais usam um controle binário explícito, que aplica somente 0 ou 1. A thread de simulação solicita `SCHED_FIFO` em todo Play; o executável pode receber somente `CAP_SYS_NICE` para essa operação, sem executar a GUI como root.

Verificação: build GUI/runner com FMILibrary externa; CTest 47/47; GUI offscreen por cinco segundos; teste documental 29/29. Não substitui a nova inspeção visual do operador, a validação de uma FMU de produto, DAQC, ESP32, XRCE ou HIL.
