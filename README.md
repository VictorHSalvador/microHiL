# FMU 2.0 Co-Simulation Real-Time Runner

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

The real-time thread never performs file I/O and never calls gnuplot. This avoids blocking the simulation because of disk or GUI latency.

## Real-time behavior

For each communication step `h`:

1. The real-time thread calls `fmi2DoStep(t, h)`.
2. Selected FMU outputs are read.
3. The step must complete before the absolute wall-clock deadline `t + h`.
4. `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)` synchronizes the result with wall time and avoids cumulative drift.
5. The sample is copied to independent SPSC queues for CSV and plotting.
6. Deadline misses and maximum computation time are reported after the run.

A simulation cannot be considered hard real-time merely because `SCHED_FIFO` is used. The FMU itself must have a bounded execution time smaller than the configured communication step, and the Linux kernel/platform must provide the required scheduling latency.

## Dependencies

Ubuntu/Debian example:

```bash
sudo apt update
sudo apt install build-essential cmake git gnuplot-qt libcap2-bin
```

### Build FMI Library

FMI Library is an open-source C importer for FMI 1.0, 2.0 and 3.0. Example installation under `$HOME/.local/fmilib`:

```bash
git clone https://github.com/modelon-community/fmi-library.git
cmake -S fmi-library -B fmi-library/build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$HOME/.local/fmilib
cmake --build fmi-library/build -j
cmake --install fmi-library/build
```

## Build this project

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DFMILIB_ROOT=$HOME/.local/fmilib
cmake --build build -j
```

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
