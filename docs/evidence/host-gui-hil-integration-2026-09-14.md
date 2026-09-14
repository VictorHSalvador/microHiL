# Integração HOST do Play HiL na GUI — 14.09.2026

## Escopo executado

O lifecycle DAQC foi extraído do `main.c` para `src/daqc_runtime.c`, preservando a sequência já exercitada pelo runner terminal. A sessão C ganhou uma operação HiL síncrona, e o controlador Qt a executa por `QtConcurrent` dentro do mesmo processo para não bloquear a thread da interface durante preflight e simulação. Terminal e GUI usam o mesmo coordenador, ponte XRCE, controle ROS, serviço serial e bridge de atuação; não há segundo processo ou segundo dono da TTY.

A GUI ganhou Play HiL separado de Play debug, campo para a porta DAQC e apresentação das estatísticas seriais disponíveis. Play HiL exige perfil YAML carregado, outputs físicos selecionados e preflight `SCHED_FIFO`; usa os valores vigentes de 152.000 bit/s, porta UDP 8888, espera XRCE 6.000 ms, confirmação ROS 100 ms e timeout CONFIG inicialmente 10 ms presentes em `AppConfig`.

## Verificações observadas

A variante ROS 2 da GUI foi configurada e compilada em `/tmp/microhil-gui-ros` com ROS 2 Humble, interfaces do projeto e FMILibrary externa. O executável iniciou por quatro segundos com `QT_QPA_PLATFORM=offscreen`, sem erro QML ou mensagem de runtime. O `RPATH` inclui as bibliotecas Humble e das interfaces MICROHIL em formato `DT_RPATH`, necessário quando o executável recebe capability de tempo real. A variante ROS do runner compilou e 49/49 CTests passaram; a variante sem ROS compilou e 48/48 CTests passaram. Em ambas, o caso de loopback UDP foi ignorado por sua pré-condição de ambiente. Também passaram 29/29 testes estruturais do SDD e 20/20 testes Python.

## Limites

O build e a inicialização offscreen não executam o botão Play HiL, não abrem `/dev/ttyUSB0` e não comprovam transições ou atuação física pela GUI. A recompilação removeu `CAP_SYS_NICE` dos artefatos temporários; o ensaio físico depende de reaplicar `cap_sys_nice=ep` ao executável da GUI. O comportamento de erros assíncronos de `/daqc_errors` durante uma simulação continua pendente de decisão: apresentação final ou interrupção imediata por classe de flag.
