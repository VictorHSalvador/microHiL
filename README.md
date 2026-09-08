# MICROHIL

O projeto está sendo retomado por desenvolvimento orientado à especificação, preservando o executor de FMU existente. A baseline vigente é a SDD-MICROHIL 0.7.2: a fundação de build HOST da TASK-001 foi implementada, evidenciada e auditada. Comece pelo [índice dos documentos](docs/README.md), pela [especificação de trabalho](docs/spec.md) e pelas [decisões](docs/decisions.md). As regras vigentes para comentários, nomenclatura e formatação estão em [AGENTS.md](AGENTS.md) e na [constituição](docs/constitution.md).

O código atual implementa um protótipo HOST com terminal, CSV e gnuplot. GUI Qt, DAQC, ROS 2/micro-ROS e comunicação física ainda não estão implementados. A [auditoria inicial](docs/avaliacao-sdd-2026-09-06.md) registra falhas de logging e limitações temporais; a [evidência](docs/evidence/audit-2026-09-06/README.md) não constitui validação HIL. Não reutilize o diretório `build/` preexistente: os comandos abaixo usam diretórios limpos e os resultados executados estão na [evidência da fundação HOST](docs/evidence/host-build-foundation-2026-09-07.md).

## Existing FMU runner

Modular C application for Linux that executes an **FMI 2.0 Co-Simulation FMU synchronized to wall-clock time**, with a dedicated POSIX real-time simulation thread, asynchronous CSV logging, and asynchronous live plotting through gnuplot.

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
|                                +----> SPSC queue ----> CSV thread|
|                                |                                 |
|                                +----> SPSC queue ----> plot thread
+------------------------------------------------------------------+
```

CSV writing and gnuplot interaction run in separate consumer threads. Error paths in the FMU wrapper still write to stderr, and FMU callbacks/internal operations have not been audited for bounded execution. The current separation is useful but does not establish complete isolation from blocking I/O.

## Real-time behavior

For each communication step `h`:

1. The real-time thread calls `fmi2DoStep(t, h)`.
2. Selected FMU outputs are read.
3. The step must complete before the absolute wall-clock deadline `t + h`.
4. `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)` synchronizes the result with wall time and avoids cumulative drift.
5. The sample is copied to independent SPSC queues for CSV and plotting.
6. The current counter reports steps already late before the sleep, and maximum computation time covers the FMU step and output reads. Wakeup lateness and the complete cycle require additional instrumentation before these values can establish timing compliance.

A simulation cannot be considered hard real-time merely because `SCHED_FIFO` is used. The FMU itself must have a bounded execution time smaller than the configured communication step, and the Linux kernel/platform must provide the required scheduling latency.

## Dependencies

Ubuntu/Debian example:

```bash
sudo apt update
sudo apt install build-essential cmake git gnuplot-qt libcap2-bin
```

## Build HOST

Use sempre um diretório novo. O build independente não procura a FMILibrary e permite compilar os componentes HOST e executar a suíte CTest:

```bash
cmake -S . -B /tmp/microhil-host-independent \
  -DMICROHIL_BUILD_RUNNER=OFF -DBUILD_TESTING=ON
cmake --build /tmp/microhil-host-independent
ctest --test-dir /tmp/microhil-host-independent -N
ctest --test-dir /tmp/microhil-host-independent --output-on-failure
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

## CSV format

```csv
sequence,simulation_time_s,wall_time_s,"output1","output2"
0,0.020000000,0.020061231,1.23,4.56
1,0.040000000,0.040057812,1.25,4.60
```

`simulation_time_s` is the FMU time. `wall_time_s` is elapsed `CLOCK_MONOTONIC` time since the start of the run and can be used to measure real-time synchronization.

## Current scope

- FMI 2.0 **Co-Simulation** only.
- Numeric output variables: Real, Integer, Boolean and Enumeration.
- Up to 64 selected outputs per run.
- FMUs returning asynchronous `fmi2Pending` from `fmi2DoStep()` are intentionally rejected.
- Input injection is not yet exposed in the terminal menu. The intended HiL extension point is immediately before `fmi2DoStep()` in `rt_simulation.c`, so DAQ/ROS data can later be copied into the real-time thread without coupling ROS or GUI code to the FMU runner.

## Production HiL notes

For a Raspberry Pi or another Linux target used as a real-time HiL host, consider a PREEMPT_RT kernel, CPU isolation/affinity, disabling frequency scaling where appropriate, and measuring worst-case scheduling latency. Do not put ROS 2 callbacks, CSV writing, terminal I/O or graph rendering in the simulation thread.
