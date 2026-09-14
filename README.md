# MICROHIL

O projeto é desenvolvido por especificação. A baseline vigente é a SDD-MICROHIL 0.27.0. O HOST já inclui executor FMI, sessão de execução compartilhada pela CLI e pela base Qt, logging binário, perfil YAML e comunicação host. O firmware ESP32 teve estados, aquisição, READ_ACK, watchdog e loopbacks de atuação verificados fisicamente, com limites documentados; a integração pelo coordenador C e a malha HIL permanecem pendentes. Comece pelo [índice dos documentos](docs/README.md), pela [especificação de trabalho](docs/spec.md) e pelas [decisões](docs/decisions.md). As regras vigentes para comentários, nomenclatura e formatação estão em [AGENTS.md](AGENTS.md) e na [constituição](docs/constitution.md).

O código atual inclui terminal de debug, logging binário, gnuplot transitório e uma base Qt Quick/QML para importar e inspecionar FMU. O suporte ROS 2/micro-ROS e os ensaios físicos estão registrados nas [evidências ESP32](docs/evidence/esp32-streaming-smoke-2026-09-13.md), [watchdog](docs/evidence/esp32-watchdog-no-ack-2026-09-13.md) e [atuação](docs/evidence/esp32-actuation-loopback-2026-09-13.md). A [auditoria inicial](docs/avaliacao-sdd-2026-09-06.md) registra limitações históricas; a [evidência da sessão](docs/evidence/host-execution-session-2026-09-10.md) delimita a verificação HOST atual. Não reutilize o diretório `build/` preexistente: os comandos abaixo usam diretórios limpos.

## Runner HOST atual

Aplicação C modular para Linux destinada a executar FMU FMI 2.0 Co-Simulation, com thread de simulação, logging binário assíncrono e plotagem separada por gnuplot. A compilação do runner não foi acompanhada de execução de FMU nesta baseline; o agendamento atual também não comprova a política final de grade fixa nem desempenho em tempo real.

## Architecture

```text
                       low priority / non real-time
                   +-------------------------------+
                   |         Terminal menu         |
                   +-------------------------------+
                                  |
                                  v
+------------------------------------------------------------------+
| Dedicated simulation thread                                     |
| pthread + SCHED_FIFO + optional CPU affinity                     |
|                                                                  |
|  input hook (future DAQ/ROS) -> fmi2DoStep() -> read outputs     |
|                                |                                 |
|                                +----> SPSC queue ----> logger binário|
|                                |                                 |
|                                +----> SPSC queue ----> plot thread
+------------------------------------------------------------------+
```

O logger binário é o consumidor proprietário do arquivo; a fila SPSC tem capacidade fixa de 128 posições, parâmetro que ainda precisa de medição. O CSV só é produzido por ação explícita depois do encerramento. Plotagem continua em consumidor separado. Ainda há `printf`/`fprintf` legados no terminal, na thread de simulação e no wrapper FMI; isso não atende a ausência final de I/O textual no caminho crítico.

## Real-time behavior

For each communication step `h`:

1. The real-time thread calls `fmi2DoStep(t, h)`.
2. Selected FMU outputs are read.
3. The step must complete before the absolute wall-clock deadline `t + h`.
4. `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)` synchronizes the result with wall time and avoids cumulative drift.
5. A amostra final é copiada para filas SPSC independentes de logging binário e plotagem.
6. The current counter reports steps already late before the sleep, and maximum computation time covers the FMU step and output reads. Wakeup lateness and the complete cycle require additional instrumentation before these values can establish timing compliance.

A simulation cannot be considered hard real-time merely because `SCHED_FIFO` is used. The FMU itself must have a bounded execution time smaller than the configured communication step, and the Linux kernel/platform must provide the required scheduling latency.

## Dependencies

Ubuntu/Debian example:

```bash
sudo apt update
sudo apt install build-essential cmake git gnuplot-qt libcap2-bin
```

## GUI Qt Quick/QML

A GUI é opcional no build e usa Qt 6. No Ubuntu 22.04, instale os arquivos de desenvolvimento e os módulos QML usados em tempo de execução:

```bash
sudo apt install qt6-base-dev qt6-declarative-dev \
  qml6-module-qtquick qml6-module-qtquick-controls \
  qml6-module-qtquick-layouts qml6-module-qtquick-dialogs \
  qml6-module-qtqml-workerscript qml6-module-qtquick-templates \
  qml6-module-qtquick-window
```

- `qt6-base-dev` e `qt6-declarative-dev` fornecem CMake, headers, compilador de recursos e bibliotecas para compilar C++/QML.
- `qml6-module-qtquick` fornece os tipos visuais fundamentais do QML.
- `qml6-module-qtquick-controls` fornece controles como `ApplicationWindow`, botões, campos e caixas de seleção.
- `qml6-module-qtquick-layouts` organiza o layout responsivo da tela.
- `qml6-module-qtquick-dialogs` fornece a seleção de FMU e YAML.
- `qml6-module-qtqml-workerscript` é uma dependência de execução do QML.
- `qml6-module-qtquick-templates` fornece a base usada pelos estilos do Qt Quick Controls 2, incluindo Fusion.
- `qml6-module-qtquick-window` fornece a janela usada indiretamente por `ApplicationWindow`.

Compile em diretório separado:

```bash
cmake -S . -B /tmp/microhil-gui -DMICROHIL_BUILD_GUI=ON -DMICROHIL_BUILD_RUNNER=OFF -DMICROHIL_FETCH_FMILIB=ON -DBUILD_TESTING=OFF
cmake --build /tmp/microhil-gui --target microhil_gui
/tmp/microhil-gui/microhil_gui
```

Para verificar apenas o carregamento em ambiente sem tela, use `QT_QPA_PLATFORM=offscreen`. O encerramento pelo `timeout` após alguns segundos indica que a janela iniciou e permaneceu ativa; não é uma validação visual nem de integração com FMU/DAQC.

## Build HOST

Use sempre um diretório novo. O build independente não procura a FMILibrary e permite compilar os componentes HOST e executar a suíte CTest:

```bash
cmake -S . -B /tmp/microhil-host-independent \
  -DMICROHIL_BUILD_RUNNER=OFF -DBUILD_TESTING=ON
cmake --build /tmp/microhil-host-independent
ctest --test-dir /tmp/microhil-host-independent -N
ctest --test-dir /tmp/microhil-host-independent --output-on-failure
```

Para exercitar também os sanitizers de endereço e comportamento indefinido no HOST, o LeakSanitizer é desabilitado neste ambiente porque ele é incompatível com `ptrace`; essa opção não é evidência de ausência de vazamentos:

```bash
cmake -S . -B /tmp/microhil-host-asan \
  -DMICROHIL_BUILD_RUNNER=OFF -DBUILD_TESTING=ON \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /tmp/microhil-host-asan
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/microhil-host-asan --output-on-failure
```

Para o runner completo, a obtenção oficial é opt-in e fixa a FMILibrary 3.0.4 na revisão `4a4b21ec10a632b2768a604c2330c54204919644`:

```bash
cmake -S . -B /tmp/microhil-runner-fetch \
  -DMICROHIL_BUILD_RUNNER=ON -DMICROHIL_FETCH_FMILIB=ON
cmake --build /tmp/microhil-runner-fetch --target fmu_rt_runner
```

Uma instalação fornecida pelo integrador também pode ser usada sem download; a versão e a compatibilidade desse artefato são responsabilidade do integrador:

```bash
cmake -S . -B /tmp/microhil-runner-external \
  -DMICROHIL_BUILD_RUNNER=ON \
  -DFMILIB_INCLUDE_DIR=/caminho/para/include \
  -DFMILIB_LIBRARY=/caminho/para/libfmilib.a
cmake --build /tmp/microhil-runner-external --target fmu_rt_runner
```

Se o runner for solicitado sem uma dependência encontrada, o diagnóstico informa as três alternativas: `FMILIB_ROOT`, `FMILIB_INCLUDE_DIR` com `FMILIB_LIBRARY`, ou `MICROHIL_FETCH_FMILIB=ON`. Compilar o runner não executa uma FMU nem comprova comportamento FMI, Raspberry Pi, DAQC, USB, ROS, GUI, concorrência real ou deadlines.

Run:

```bash
./build/fmu_rt_runner
```

## Permission for SCHED_FIFO

A normal Linux user commonly receives `EPERM` when requesting `SCHED_FIFO`. A convenient development option is to grant only `CAP_SYS_NICE` to the executable:

```bash
sudo setcap cap_sys_nice+ep ./build/fmu_rt_runner
getcap ./build/fmu_rt_runner
```

Then run it as the normal desktop user, which also avoids GUI problems with gnuplot caused by launching the whole application through `sudo`.

If **Strict RT** is disabled, the application warns and falls back to the normal scheduler. If Strict RT is enabled, simulation aborts when `SCHED_FIFO` cannot be activated.

## Suggested first test

For an FMU exported from OpenModelica:

- Step size: `0.02 s`
- Duration: `30 s`
- SCHED_FIFO priority: `80`
- CPU affinity: a dedicated core if available
- Plot window: `10 s`
- Plot refresh: `0.10 s`
- CSV enabled

The terminal menu will import the FMU, inspect `modelDescription.xml` through FMI Library, list numeric output variables, and allow selection by index.

## Fluxo de registro e conversão

Durante uma execução com logging habilitado, o runner grava `MHILLOG1` little-endian, com SHA-256 dos bytes da FMU e descritor canônico das saídas. O índice XML é one-based, isto é, a posição da `ScalarVariable` na lista do XML. O formato preserva Real, Integer, Enumeration e Boolean, bitmap de qualidade e valores finais por passo; não inclui métricas temporais por registro.

Após o encerramento, o menu de debug pode converter o último log fechado para CSV. O conversor reconstrói o descritor da FMU carregada e exige igualdade de hash, índice XML, nome, tipo, valueReference e ordem. `t_start` e `h` ficam registrados no arquivo, mas não integram essa identidade. As colunas são `sequence`, `simulation_time_s` e, para cada saída, `<nome>_value`, `<nome>_valid`. Real inválido vira `NaN`; discreto inválido, zero com qualidade `0`. Log marcado incompleto não é disponibilizado como log íntegro para conversão.

## Current scope

- FMI 2.0 **Co-Simulation** only.
- Numeric output variables: Real, Integer, Boolean and Enumeration.
- Up to 64 selected outputs per run.
- FMUs returning asynchronous `fmi2Pending` from `fmi2DoStep()` are intentionally rejected.
- Input injection is not yet exposed in the terminal menu. The intended HiL extension point is immediately before `fmi2DoStep()` in `rt_simulation.c`, so DAQ/ROS data can later be copied into the real-time thread without coupling ROS or GUI code to the FMU runner.

## Production HiL notes

For a Raspberry Pi or another Linux target used as a real-time HiL host, consider a PREEMPT_RT kernel, CPU isolation/affinity, disabling frequency scaling where appropriate, and measuring worst-case scheduling latency. Do not put ROS 2 callbacks, CSV writing, terminal I/O or graph rendering in the simulation thread.
