# Ciclo DAQC no runner HOST — 10.09.2026

- Ambiente: Ubuntu 22.04; ROS 2 Humble; interfaces `microhil_interfaces` compiladas localmente; GCC 11.4; CMake; FMILibrary obtida pelo build.
- Escopo: integração condicional `MICROHIL_WITH_ROS2_CONTROL` no `fmu_rt_runner`, timeout ROS configurável de 100 ms, preflight SCHED_FIFO e ligação da bridge de atuação ao ciclo FMI.

## Procedimento executado

1. Carregar ROS 2 Humble e a instalação local das interfaces.
2. Compilar o runner com `MICROHIL_WITH_ROS2_CONTROL=ON` e testes habilitados.
3. Executar a suíte CTest completa da variante ROS.
4. Executar o teste estrutural do SDD e a checagem de espaços em branco do diff.

## Resultado observado

O build concluiu e o CTest ROS concluiu **47/47** testes. O runner contém o caminho de Play que, quando a integração DAQC é habilitada, verifica SCHED_FIFO antes de STREAMING, inicia coordenador, ponte UDP, TTY e controle `rcl`, confirma ENABLE/configuração por `DaqcState` dentro de `daqc_ros_timeout_ms`, inicia STREAMING e conecta a bridge de outputs à thread FMI. Encerramento solicita DISABLE antes de parar os workers.

O teste de FMU também rejeitou uma configuração de prioridade inválida no preflight. Não foi possível afirmar que SCHED_FIFO foi concedido para uma execução HiL porque a suite não habilitou o enlace DAQC nem executou a política no alvo.

## Limites

Não houve Agent conectado à ponte junto do runner, sessão XRCE, CH340, ESP32, DAQC, TTY físico, tópico remoto, CONFIG físico, FMU executada com a DAQC, GPIO, bancada, HIL ou medição de interferência e deadlines. A execução não demonstra que uma confirmação ROS chegará em 100 ms no enlace real; esse valor é uma configuração inicial aprovada, a ser qualificada em bancada.
