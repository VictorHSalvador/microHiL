# Evidência HOST — Play debug consecutivo

Data: 12.09.2026. Ambiente: Ubuntu 22.04, GCC 11.4.0, CMake, Qt 6.2.4 e FMILibrary externa já configurada no diretório de build `/tmp/microhil-gui-fix`. Escopo: software HOST.

## Procedimento

1. Compilar a GUI e os testes no diretório de build existente.
2. Executar o CTest completo. O teste `execution_session_lifecycle` inicia, junta e encerra duas execuções consecutivas da mesma sessão, com logging binário e fila de gráficos habilitados; em cada execução ele consome uma amostra e verifica duas etapas concluídas.
3. Iniciar a GUI com `QT_QPA_PLATFORM=offscreen` por cinco segundos para verificar o carregamento QML.
4. Executar a verificação documental.

## Resultado observado

A compilação foi concluída. Os 47 CTests passaram; o teste XRCE de loopback foi pulado por sua pré-condição de ambiente. A GUI iniciou em modo offscreen sem erro de carregamento QML. A verificação documental retornou 29 testes aprovados.

A interface limpa as amostras de cada janela de gráfico ainda aberta somente após `StartSimulation` retornar sucesso. Assim, uma tentativa de Play recusada preserva a visualização anterior; uma nova execução aceita inicia os gráficos sem dados da execução anterior. A sessão C já reinicializa a FMU, logger e fila no novo Play, e agora há teste de duas execuções consecutivas.

## Limites

Não houve interação visual manual nesta execução, nem validação com FMU de produto, DAQC, ESP32, CH340, Agent, ROS, Raspberry Pi, bancada, HIL ou qualificação temporal. O teste HOST não prova a ausência de qualquer crash específico do compositor ou do ambiente gráfico do operador.
