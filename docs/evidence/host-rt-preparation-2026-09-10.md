# Preparação da FMU antes da thread de simulação — 10.09.2026

- Ambiente: Ubuntu 22.04; GCC 11.4; CMake; FMILibrary 3.0.4 na revisão fixa `4a4b21ec10a632b2768a604c2330c54204919644`.
- Escopo: `rt_simulation_prepare`, referências iniciais dos inputs e início posterior da thread, usando a fixture `tests/fixtures/teste001.fmu`.

## Procedimento executado

1. Carregar a fixture, listar suas entradas e saídas e criar uma configuração de dois passos de 10 ms.
2. Chamar `rt_simulation_prepare` e verificar que o estado de input está pronto, que a FMU está inicializada e que o contador de passos ainda é zero.
3. Iniciar a thread preparada, aguardar seu término e conferir dois passos concluídos.
4. Compilar o runner com FMILibrary e executar a suíte CTest completa.

## Resultado observado

A preparação resolveu as referências iniciais sem executar `doStep`. Após o início explícito da thread, a fixture concluiu os dois passos previstos. A suíte CTest concluiu **46/46** testes com sucesso.

## Limites

O teste usa uma fixture FMU e não cria coordenador DAQC, TTY, CH340, Agent, sessão XRCE, ROS 2, ESP32, aquisição física, transição CONFIG, bancada ou HIL. Também não qualifica SCHED_FIFO, deadline, jitter, latência ou o comportamento elétrico das saídas.
