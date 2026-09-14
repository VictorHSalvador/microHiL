# Ensaio físico do runner C com DAQC e FMU — 14.09.2026

## Objetivo e ambiente

O ensaio verificou o ciclo integrado em que `fmu_rt_runner` é o único dono da CH340, encaminha XRCE ao Micro-ROS Agent UDP local, configura a DAQC por ROS 2, executa uma FMU e encerra o hardware em DISABLE. O ambiente foi Ubuntu 22.04 em PC x86-64, ROS 2 Humble, ESP32-D0WD-V3 pela CH340 em `/dev/ttyUSB0`, UART 152.000 bit/s 8N1 e firmware físico previamente gravado da revisão 0.26.7. O usuário informou os loopbacks D16→D4, D25→D32 e D18→D33.

O Agent foi reconstruído depois da reinicialização do computador com `micro_ros_msgs` no commit `c9062eb3860d16c1bff1423923de3b0956fd4734`, `micro-ROS-Agent` no commit `c93ee764e0d2ef4907aeb29233c68cb5f4b56976` e Micro XRCE-DDS Agent transitivo no commit `57d086216d01ec43121845d385894a25987f8a2c`. O runner usou `tests/fixtures/teste001.fmu`, `tests/fixtures/esp32_profile.yaml`, passo de 0,01 s, duração de 0,2 s, porta UDP 8888, espera inicial ROS de 6.000 ms, confirmação ROS de 100 ms e CONFIG UART de 100 ms para este ensaio diagnóstico. O valor padrão CONFIG do produto permanece 10 ms.

## Correção verificada antes do ensaio

O caminho Agent→DAQC mantinha somente o datagrama XRCE mais novo e apagava o pendente quando chegava uma confirmação CONFIG. Um teste do coordenador que enfileira dois datagramas reproduziu a perda do primeiro antes da correção. A implementação passou a usar FIFO limitada de 64 datagramas, preservar ordem e contabilizar saturação; a confirmação CONFIG deixou de limpar essa fila porque a sessão XRCE é independente da máquina de estados CONFIG.

A abertura física da TTY também passou a desabilitar DTR/RTS e, quando a opção de preflight está habilitada, reproduzir a sequência de reset já usada pelo harness: DTR inativo, RTS ativo por 100 ms, RTS inativo e estabilização por 1.000 ms. O reset ocorre antes do descarte de RX e da confirmação DISABLE e é configurável, com valor inicial habilitado.

## Procedimento e resultado observado

O Agent e o runner foram mantidos pelo mesmo processo supervisor durante todo o ensaio. O executável recebeu `CAP_SYS_NICE` e confirmou `SCHED_FIFO` ativo. O log do Agent registrou `create_client`, sessão estabelecida para a chave `0x4D48494C`, participante, subscriber e dois publishers. O runner completou preflight DISABLE, ENABLE, `DaqcSetup`/`DaqcState`, STREAMING, execução FMI e encerramento DISABLE, terminando com código zero.

Resultado final do runner:

```text
State: FINISHED
Code: 0
Stage: complete
Message: simulation completed
SCHED_FIFO active: yes
Completed steps: 20
Deadline misses: 0
Unused releases: 0
Max FMU computation time: 0.025190 ms
Max deadline lateness: 0.000000 ms
DAQC RX bytes: 21705
DAQC TX frames: 734
DAQC read timeouts: 948
DAQC I/O failures: 0
```

Antes da atualização documental, o build limpo do runner concluiu e o CTest aprovou **49/49** testes, incluindo FIFO XRCE, preservação após CONFIG, reset TTY, ponte UDP, controle ROS e execução FMI. A suíte Python aprovou **20/20** testes e a suíte documental vigente aprovou **29/29** testes.

## Limitações

O ensaio comprova uma execução curta do ciclo C–Agent–DAQC–FMU no hardware conectado. Ele não registrou os valores de entrada e saída por canal durante a execução; portanto, os jumpers informados não constituem evidência de correspondência numérica, precisão de ADC/DAC, duty intermediário do PWM, carga elétrica ou malha fechada correta. A duração de 0,2 s e as 20 etapas sem deadline perdido não qualificam operação contínua a 100 Hz, jitter, carga, Raspberry Pi ou hard real-time. O número elevado de timeouts de leitura é uma métrica de polling limitado do worker e requer interpretação em ensaio temporal, sem representar por si só perda de DATA.
