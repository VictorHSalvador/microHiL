# Evidência HOST — resultado e CSV na GUI

Data: 11.09.2026. Ambiente: Ubuntu 22.04, GCC 11.4.0, CMake, Qt 6.2.4 e FMILibrary 3.0.4. Escopo: software HOST.

## Procedimento executado

O teste `execution_session_lifecycle` selecionou as saídas da fixture FMU, executou uma sessão curta pela API usada pela GUI, aguardou o encerramento, leu as métricas agregadas e registrou as saídas no log binário temporário. Em seguida, converteu esse arquivo fechado para CSV pelo mesmo ponto de entrada exposto à GUI e confirmou dois registros exportados sem parcialidade.

A GUI Qt Quick/QML foi recompilada e iniciou em modo `offscreen`. Ela consulta o resultado somente após o `Join`, apresenta passos, perdas de deadline, liberações não usadas e piores tempos, e abre o seletor de destino CSV apenas quando o log binário foi fechado sem incompletude.

## Resultado observado

**47/47 testes CTest passaram; 1 teste XRCE foi pulado por não estar habilitado neste build.** A compilação GUI terminou e o processo permaneceu ativo pelo período offscreen, sem erros QML.

## Limites

Não houve inspeção visual humana, execução da FMU de produto, teste manual do seletor de arquivo, DAQC, CH340, ESP32, Agent micro-ROS, ROS 2 em enlace, Raspberry Pi, bancada, HIL ou medição temporal de aceitação.
