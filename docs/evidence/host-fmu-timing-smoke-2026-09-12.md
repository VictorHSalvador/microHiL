# Execução HOST de FMU e timing — 12.09.2026

Ambiente: Ubuntu 22.04, build CMake limpo em `/tmp/microhil-runner-manual`, FMILibrary externa encontrada em diretório local do integrador, e fixture `tests/fixtures/teste001.fmu`.

Procedimento: compilar `fmu_rt_runner`; importar a FMU; selecionar as três saídas numéricas; configurar passo de 10 ms e duração de 100 ms; desabilitar log binário e gnuplot; iniciar a execução HOST.

Resultado observado: a FMU terminou 10 passos, sem deadlines perdidos ou liberações não usadas. O maior tempo de computação observado foi 0,026663 ms. `SCHED_FIFO` não foi concedido ao processo e o runner continuou porque a execução não era HiL e o modo estrito estava desabilitado.

Limites: é uma única execução de fixture em máquina Linux; não qualifica WCET, jitter, escalonamento, Raspberry Pi, DAQC, ROS, micro-ROS, sinais elétricos ou HIL.


## Logging e CSV

Uma segunda execução com o mesmo passo e duração habilitou o log binário. Foram aceitas e persistidas 10 amostras, sem descarte; o último `sequence` foi 9. Após o fechamento, a conversão produziu `/tmp/microhil-timing.csv` com cabeçalho tipado e 10 registros. O resultado permanece limitado a HOST e não qualifica o caminho HiL.
