# Evidência HOST — seleção de saídas pela GUI

Data: 10.09.2026. Ambiente: Ubuntu 22.04, GCC 11.4.0, CMake, Qt 6.2.4 e FMILibrary 3.0.4 na revisão imutável configurada pelo projeto. Escopo: software HOST.

## Procedimento executado

Foi compilada uma árvore temporária com runner, FMILibrary e testes. O teste `execution_session_lifecycle` carregou a fixture FMI 2.0 Co-Simulation, enumerou três saídas numéricas, verificou a contagem exposta à GUI, selecionou uma saída, removeu a seleção e selecionou todas antes de iniciar o ciclo curto.

Em árvore separada, a GUI Qt Quick/QML foi compilada e iniciada por três segundos em `QT_QPA_PLATFORM=offscreen`. A lista QML deriva da propriedade de caminho da FMU e chama apenas a API de `execution_session` para consultar ou modificar a seleção.

## Resultado observado

A compilação HOST foi concluída, **47/47 testes CTest passaram** e o teste documental `node test/run_spec_tests.js` passou em **29/29** critérios. A execução offscreen permaneceu ativa até o timeout esperado, sem o aviso anterior de manipulador `Connections` sem sinal correspondente.

## Limites

O modo offscreen não verifica composição visual, interação humana, acessibilidade ou aspecto Linux Mint. Não foram executados editor de mapeamento, entradas virtuais, geração/escrita YAML, Play/Stop pela GUI, gráficos, conversão CSV pela GUI, DAQC, CH340, ESP32, Agent micro-ROS, ROS 2 em enlace, bancada ou HIL. Os avisos de compilação vieram de headers de terceiros da FMILibrary.
