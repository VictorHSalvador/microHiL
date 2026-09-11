# Arquitetura de software

Revisão 0.23.4: código atual, decisões confirmadas e design do ICD consolidados. Requisitos em [spec.md](spec.md); parâmetros de implementação/ensaio restantes em [decisions.md](decisions.md). A sessão de execução compartilhada está materializada e testada no HOST; a GUI também usa sua API para listar e selecionar saídas numéricas da FMU. Isso não declara GUI operacional, DAQC física ou qualificação temporal.

## ARCH-CORE — Núcleo, FMU e configuração

Preservar `src/fmu_model.c`, `src/rt_simulation.c`, `src/app_config.c` e headers como base. O wrapper já importa FMI 2.0 CS, enumera outputs e encapsula instância. Deve evoluir para validar inputs, tipos, unidades e mapeamento antes de executar. A capacidade de último passo variável precisa ser considerada: o exemplo permite, mas o loader aceita outros modelos. A meta é interpretar qualquer FMU 2.0 CS; Q-08 autoriza reconhecer e diagnosticar capacidades não suportadas, como Pending/String quando fora do incremento. Não declarar suporte universal enquanto o wrapper os rejeita ou ignora. Formato compatível não comprova binário compatível com CPU nem custo computacional dentro do passo.

Qt e terminal debug desacoplados do núcleo foram confirmados. `execution_session` materializa a orquestração reutilizável: prepara a FMU, inicia logging, inicia a thread de simulação, recebe Stop e agrega o resultado. Terminal e GUI usam a mesma sessão; a GUI importa FMU e YAML, aplica controles virtuais, executa o fluxo debug HOST, consome amostras e consulta o resultado somente após o Join. Configuração YAML é validada e congelada por execução; valores de controles virtuais são operacionais, não persistem no YAML, chegam por comando e são aplicados na fronteira de ciclo. Nenhum callback GUI/ROS chama a FMU diretamente.

| Módulo atual | Preservar | Ajustar/estender |
|---|---|---|
| app_config | Dados de execução | Validação finita, faixa, coerência e perfil |
| fmu_model | Importação, lifecycle, tipos de outputs | Inputs, capacidade/arquitetura, fixtures, falhas e diagnóstico limitado |
| rt_simulation | Thread dedicada, relógio monotônico absoluto | Estado, wakeup de stop, métricas completas, entradas/saídas consistentes |
| execution_session | Sessão de FMU, ciclo e resultado comum a CLI/GUI | Expor comandos e eventos não bloqueantes para Qt, sem acesso direto à FMU |
| main | Fluxo CLI útil para diagnóstico | Delegar toda a execução à sessão e manter somente interação/diagnóstico |
| sample_queue | Cópia SPSC e atomics | Testes concorrentes, limites e política por consumidor |
| csv_logger | Consumidor assíncrono | Falhas/encerramento como reuso; novo sink binário tipado e CSV apenas pós-execução |
| plotter | Prova de desacoplamento | Instrumento transitório até Qt; não satisfaz janelas individuais/controle ao vivo |

## ARCH-STATE — Comandos, estados e falhas

Estados host e estados DAQC são máquinas distintas. Host preserva Idle/Running/Error/Stopped/Finished. DAQC tem DISABLE, ENABLE (IDLE) e STREAMING conforme DEC-012. Não confundir ENABLE/IDLE físico com tela Idle.

| Evento | Comportamento confirmado | Detalhe ainda aberto |
|---|---|---|
| Carregar FMU | Botão/diálogo próprio; validar FMI 2.0 CS e analisar nomes/tipos | Matriz de capacidades do incremento; diagnóstico autorizado em Q-08 |
| Carregar configuração | Outro botão; comparar FMU/DAQ/mapa e parâmetros, listar divergências | Identidade/versão no arquivo, Q-07/Q-09 |
| Play | Inicializar FMI e inputs antes de STREAMING; em modo DAQC, validar SCHED_FIFO, iniciar coordenador/TTY/ponte UDP/nó ROS, confirmar ENABLE/configuração em `DaqcState`, solicitar STREAMING e só então criar a thread FMI; DATA somente após Play | Falha de lifecycle produz Error antes de `doStep`; amostras inválidas seguem F-24 |
| Running + deadline perdido | Continuar, contar e guardar pior atraso | Agenda posterior, Q-06 |
| Stop/fim normal/erro | Zerar saídas físicas e encerrar DATA; preservar registros disponíveis | Falha de entrega deve ser diagnosticada; novo Play reinicializa outputs pela FMU |
| Aquisição DAQC inválida | Host ignora o bruto inválido e retém último válido no input FMU | Antes do histórico, usar valor inicial válido do input FMU; não zerar atuadores |
| 100 passos consecutivos de aquisição inválida + checkbox | Terminar em Error no 100º, identificar canais DAQC/inputs FMU afetados e permitir novo Play | Contagem por canal e por passo, não por pacote; válido quebra sequência |
| Fechar aplicação normalmente | DAQC deve aceitar DISABLE mesmo em STREAMING e liberar fluxo | Prazo agregado de controle será medido |
| STREAMING sem progresso de leitura host por 60 s | DAQC entra em DISABLE automaticamente, sem etapa de zero | READ_ACK MID 03; cadência e tolerância serão medidas |
| Boot/reset DAQC | Não transmitir DATA até Play; limpar mailboxes e sequências | Não deduzir estado elétrico da retenção no host; TARGET trata atuadores |

Não há pausa manual nem automática: Q-03 escolheu Error e novo Play. Ao repetir a simulação, reinicializar lifecycle/configuração e não herdar contadores e resultados sem contrato explícito.

Manter no host, por canal de aquisição, valor bruto/qualidade e último válido utilizado no input FMU. Antes da primeira aquisição válida, usar valor inicial válido desse input. Snapshot deve associar esses dados de modo coerente. Falha de aquisição não manda zero ao mundo real. Outputs FMU e AO/DO/PWM seguem caminho e contrato próprios; não confundir leitura USB com confirmação de atuação.

Diagnósticos próprios e callbacks de erro da FMU são encaminhados à GUI fora do caminho crítico. Terminal só em debug, também por consumidor não crítico; nenhum printf por ciclo. Código legado contém stderr e ainda será corrigido. Mensagens emitidas diretamente por binário externo não são automaticamente controladas pelo flag do aplicativo.

Observação de arquivo: a FMU atual tem 14 outputs e 12 não possuem start explícito no XML (inspeção sem execução). Ler start literal não é suficiente para inicializar todos os modelos. O [schema FMI 2.0.4](https://raw.githubusercontent.com/modelica/fmi-standard/v2.0.4/schema/fmi2ScalarVariable.xsd) distingue inicialização exact/approx/calculated. Q-04 confirma permitir inicialização sem start literal; usar valor calculado válido após lifecycle e Play. Erro retornado pela API FMI não pode ser ignorado para continuar doStep.

## ARCH-TIME — Ciclo e orçamento temporal

Meta confirmada: operar pelo menos a 100 Hz para modelos cujo custo caiba no alvo, sem fixar planta; passo e duração vêm da GUI/terminal. 100 Hz corresponde a 10 ms. Não prometer 100 Hz para toda FMU importável. Ter fixtures de benchmark para ensaio não restringe o produto a uma planta.

Fluxo de design: receber snapshot DAQC → selecionar/validar candidato por canal e atualizar contador uma vez por passo → usar último válido ou referência inicial válida no input FMU → verificar proteção → aplicar inputs → doStep → ler outputs FMU → publicar atuação conforme contrato próprio → enfileirar outputs para log → agendar. A aquisição USB é assíncrona; não esperar confirmação de leitura dentro do núcleo. O contador e o snapshot exigem trabalho limitado proporcional ao número de canais configurados, sem I/O textual ou alocação por ciclo.

Confirmado: grade fixa de liberações no relógio real; preservar h e a sequência do tempo simulado, sem saltar etapas FMI, sem compensar em rajadas e sem deslocar a origem da grade. Se o cálculo atravessar uma liberação, aguardar o próximo instante fixo disponível. Exemplo aprovado: h=10 ms, etapa iniciada em 0 e concluída em 12 ms → próxima etapa em 20 ms. Não iniciar em 12 ms para compensar nem mudar h. O tempo simulado pode ficar atrás do relógio real; não ocultar essa diferença.

| Item | Confirmado ou pendente |
|---|---|
| Passo e duração | Configuráveis; positivos/finitos, coerentes com capacidades da FMU |
| Referência de desempenho | 100 Hz / 10 ms; não é passo fixo obrigatório |
| Overrun | Aguardar próximo instante da grade fixa, sem saltar etapas FMI; perdas e pior atraso exibidos após execução |
| Timeout USB | Máximo 5 ms por transferência, confirmado em Q-06. API configurada não prova limite real sob scheduler |
| Renderização | Até 10 Hz, desacoplada; frames gráficos podem ser coalescidos sem descartar amostras do log |
| SCHED_FIFO | Obrigatório no Play HiL; preflight em thread dedicada verifica a política antes de STREAMING e a thread FMI a configura novamente. Se a segunda configuração falhar, o run termina em erro, sem fallback silencioso |
| Idade de entrada/cadência de aquisição | Último dado válido reutilizável até fim; valor constante é válido. Cadência de aquisição ainda a dimensionar; 60 s sem leitura host seguem F-27 |
| Jitter aceitável/duração/carga de ensaio | Detalhar na verificação; limites não vieram do Demo |

Duas esperas sequenciais de 5 ms já consomem 10 ms antes de FMU/filas/overhead. O enlace serial é UART 8N1 configurável de 9.600 a 152.000 bit/s, com RTS/CTS desabilitado; a baseline selecionada é 152.000 bit/s. Em 152.000 bit/s, 261 bytes exigem aproximadamente 17,17 ms apenas no fio e um frame XRCE máximo de 133 bytes, com MTU 128, exige cerca de 8,75 ms. Isso impede usar o DATA máximo em 100 Hz e é um limite aritmético, não medição; o perfil deve validar tamanho de payload e orçamento completo antes de STREAMING. XRCE é best effort e não deve iniciar quando seu envio violar o orçamento do tráfego crítico. Não alterar `h` nem compensar etapas para ocultar essa limitação.

Coletar médias/máximos de ciclo, FMU e leitura/escrita, contagem de timeouts e pior atraso para apresentação final. Esses agregados ficam separados do stream binário de saídas. Escopo exato de persistência de metadados está em IF-LOG. Não fazer I/O textual/disco/GUI/ROS síncrono no ciclo, nem alocar snapshots Python na thread crítica C por analogia com RaspDAQ.

## ARCH-IO — Comunicação e perfis

A USB-C da placa liga host → CH340 → UART do ESP32; não é uma interface nativa USB de ESP32-S3. O coordenador C usa exclusivamente `/dev/ttyUSB*`, configurado pelo driver CH341 Linux. O worker `daq_serial_service` implementa RX/TX limitado em thread própria no HOST; a [evidência](evidence/host-serial-service-2026-09-10.md) cobre pseudo-terminal. O coordenador já demultiplexa XRCE recebido e mantém um mailbox XRCE de saída limitado a 128 bytes; a ponte UDP em loopback foi verificada com socket local, mas a execução junto ao Agent ainda permanece pendente.

RaspDAQ foi localizado em Projects/OT1-HiLInfrastructure e inspecionado: raspdaq_main cria uma única SharedDaqState, passada ao runtime FunctionFS e ao nó rclpy. Snapshots imutáveis são substituídos sob RLock; o objeto compartilhado/nó permanece. Reaproveitar ownership, troca de snapshots e coordenação de encerramento, sem copiar endpoints Linux FunctionFS para ESP32. A camada micro-ROS/XRCE continua necessária. [Inspeção estática](evidence/q-review-2026-09-06.md).

ADR-003 usa a referência para definir dono único e snapshots. O ICD reserva MID 04 para o transporte XRCE e MID 03 para READ_ACK, lacunas não cobertas pelo RaspDAQ. A seleção micro-ROS de perfil ocorre fora de STREAMING por `DaqcSetup.profile_id`; o perfil compilado valida e congela seu schema antes de atuar, e `DaqcState` confirma a aplicação. Não deixar o coordenador e um agente serial padrão lerem concorrentemente a mesma porta. O MID 04 atravessa uma ponte UDP de loopback até o Agent, que usa UDP e nunca abre a CH340. Qt Quick/QML chama somente interfaces de controle e nunca a FMU. Amostras de saída seguem por fila SPSC e são consumidas pela GUI em no máximo 10 Hz.

Design vigente: dono único do enlace, demultiplexação CONFIG/DATA/READ_ACK/XRCE no adaptador, snapshots completos com geração publicados sob lock limitado, núcleo só copia estruturas internas; ninguém mantém mutex durante I/O/ROS. DATA usa mailbox de última atualização, sem fila crescente nem retransmissão. Double-buffering exigido por NF-08 continua base a reconciliar com objeto imutável do serviço.

O controlador HOST de estados aguarda confirmação somente fora da thread de simulação. CONFIG UART usa `daqc_config_timeout_ms`, inicialmente 10 ms; a confirmação ROS de `DaqcSetup` usa `daqc_ros_timeout_ms`, inicialmente 100 ms. Play HiL executa o preflight SCHED_FIFO, solicita CONFIG ENABLE, publica `DaqcSetup` e exige `DaqcState` ENABLE com perfil/configuração aplicados antes de solicitar STREAMING e criar a thread FMI. Stop, término e Error solicitam DISABLE após congelar a produção; a confirmação ausente é reportada, sem alterar o relógio do modelo.

Perfil ESP32 tem mapa funcional confirmado: AI em GPIO32/33/34/35/36/39, DI em GPIO4/13/14/27, AO em GPIO25/26, DO em GPIO16/17/21/22/23 e PWM em GPIO18/19. UART0 (GPIO1/3) e pinos de boot (GPIO2/5/12/15) ficam fora do perfil. DI/AI da DAQC alimentam inputs da FMU; outputs FMU destinados ao hardware alimentam DO/AO/PWM. O mapa analógico usa volts; DAC converte explicitamente para 0…255. PWM usa duty normalizado no DATA e frequência/resolução validadas por `DaqcSetup` fora de STREAMING. Unidades e faixas precisam de conversão explícita: real da FMU não significa volts por definição. `Integer` e `Enumeration` não são mapeáveis ao perfil físico atual; a GUI os trata como virtuais ou de observação. ICD em [interfaces.md](contracts/interfaces.md).

## ARCH-GUI — Interface e persistência

Qt 6/C++ confirmado, desacoplado do núcleo; terminal debug é alternativa operacional. Importar FMU e carregar configuração usam botões/diálogos separados. Uma configuração incompatível informa variável/tipo/perfil/parâmetro divergente e não é aplicada parcialmente. GUI solicita comandos, não acessa instância FMI nem bloqueia thread crítica.

Direção visual confirmada: Linux Mint. Proposta de paleta inicial (design, não cores oficiais exigidas): superfícies claras cinza, texto grafite e verde para seleção/ação primária; alertas em âmbar/vermelho com texto/ícone, sem depender só de cor. Estilo próprio C++ foi registrado na constituição preservando APIs Qt.

| Controle/comportamento | Contrato confirmado |
|---|---|
| Play/Stop | Independentes da criação dos gráficos; checkbox de proteção de inválidos; sem pausa manual |
| Abrir gráfico | Um por saída, possível antes de Play; janela vazia até receber amostras |
| Configurar gráfico | Controle distinto da abertura; mínimo, máximo e espaçamento Y |
| Y −2…2, espaçamento 1 | Marcações −2, −1, 0, 1, 2; não é quantidade de amostras |
| Tempo | Janela única comum; ao exceder limite superior, avançar limites inferior/superior mantendo extensão configurada |
| Atualização | No máximo 10 Hz, prioridade não RT; não altera taxa de aquisição/simulação/log |
| Fechar | Destruir gráfico e histórico próprio, sem parar log ou simulação |
| Reabrir durante run | Coletar a partir do instante de reabertura, sem preencher com histórico antigo |

Renderização a 10 Hz não implica guardar só 10 valores/s: gráfico aberto pode receber as amostras por passo e renderizar um lote; memória limitada à janela. Fechar histórico não impede preservar parâmetros de configuração. Quantidade máxima de pontos/memória deve ser dimensionada separadamente do espaçamento Y.

Log binário de saídas finais por passo; conversão CSV após encerramento, utilizando FMU para interpretar tipos. Não converter durante Running. A configuração persistida é YAML versionado; o log permanece binário. IF-LOG trata identidade/ordem, layout e política de falha; métricas por passo saem do log por instrução do usuário.

## ARCH-LOG — Dados e concorrência

| Dado | Proprietário proposto | Acesso |
|---|---|---|
| Configuração validada e mapa | Controlador da execução | Imutável durante run; cópia/configuração versionada |
| Instância e tempo FMU | Controlador prepara; thread de simulação executa | Inicialização e referências de input são resolvidas antes do STREAMING; `doStep` e leituras/escritas FMI do ciclo pertencem exclusivamente à thread |
| Controle ROS 2 | Thread `rcl` do runner | Publica setup e recebe estado/erros; não chama FMI, não possui TTY e não bloqueia a thread de simulação |
| Input snapshot | Adaptador USB host | Candidato bruto, qualidade e último válido por canal, geração/seq coerentes; simulação consome sem esperar I/O |
| Contadores de aquisição inválida | Thread de simulação no host | Atualiza uma vez por passo por canal; publica erro para consumidor GUI |
| Progresso de leitura | Thread de comunicação host envia READ_ACK; firmware supervisiona | Último SEQ lido no STREAMING atual, independente de valor numérico/constância |
| Comandos virtuais | Controlador aceita; simulação aplica | Publicação limitada e confirmação de aplicação na fronteira de ciclo |
| Output snapshot | Simulação | Cópias para adaptador de saída; sem ponteiros mutáveis para widgets |
| Filas log/plot | Simulação produz, um consumidor por fila | SPSC enquanto topologia for exatamente essa; não adicionar consumidor à mesma fila sem revisão |
| Estado público | Controlador da execução | Eventos ordenados e snapshots; erro dos workers propagado |
| Atuação física AO/DO/PWM | Tarefa DAQC com escritor final único | Dados somente STREAMING; encerramento aplica zero e cessa DATA; novo run aplica outputs iniciais da FMU |

Históricos gráficos são limitados à janela e à vida do gráfico; renderizações podem ser coalescidas. Q-07 determina continuar a simulação com aviso e registro incompleto quando o logger falhar. Contabilizar descartes detectáveis; falha de escrita pode deixar persistência final indeterminada, nunca alegar gravação integral. Essa diferença precisa de contrato, sem espera ilimitada na simulação. Encerramento deve drenar após observar fim de produção e verificar novamente a fila; erro de escrita/close deve chegar ao resultado agregado.

No HOST auditado, cada fila ocupa 2.228.248 bytes e há duas na pilha de main. O histórico de plot tem 5000 amostras. Dimensionar buffers, pilhas e atomics no alvo, com limites que ainda não existem; não inferir orçamento do firmware a partir dessa alocação Linux.

## ARCH-FW — Firmware proposto

O firmware possui uma base compilada para ADC/DAC internos, perfil ESP32 e capacidades do TARGET. Estados obrigatórios DISABLE, ENABLE (IDLE), STREAMING; parser atende CONFIG em todos eles e não fica preso em transmissão contínua. DISABLE deve interromper streaming sem matar a capacidade de receber futuros comandos. CONFIG é a única autoridade de transição. `/daqc_setup` só confirma o estado já efetivo e configura perfil/ADC/PWM fora de STREAMING; divergência de `command` é diagnosticada por `/daqc_errors`. A aplicação micro-ROS de `/daqc_setup`, `/daqc_state` e `/daqc_errors` usa MID 04 e permanece fora do caminho de aquisição/atuação.

No host, o runner hospeda um nó `rcl` em thread própria para esse mesmo conjunto de tópicos. A thread recebe e guarda o último estado/erro com sincronização limitada; a thread de simulação não participa de ROS. O controlador de ciclo pode aguardar confirmação fora do caminho crítico antes de STREAMING, mas não aguarda publicação ROS durante `doStep` nem para encerrar uma simulação já em curso.

Separar aquisição/atuação, parser, controle de estado, diagnóstico e micro-ROS. Usar os dois núcleos do ESP32; proposta: aquisição/atuação periódica em um núcleo e comunicação/micro-ROS/supervisão no outro. Índices de CPU, afinidades de interrupções, prioridades, clocks, RTOS/versão e orçamento ainda serão fixados após identificar tarefas do SDK. Não prometer isolamento total: memória/periféricos e sincronização continuam compartilhados. Não copiar os números do Demo.

A implementação inicial fixa aquisição/atuação no núcleo 1 com prioridade 9, comunicação no núcleo 0 com prioridade 8 e `daqc_ros` no núcleo 0 com prioridade 4 e pilha de 6144 bytes. Esses são parâmetros de implementação para manter ROS abaixo do tráfego crítico, não evidência de orçamento, ausência de inversão de prioridade ou cumprimento de deadline; a qualificação deve medi-los no alvo.

UART0 usada para dados não pode misturar logs de debug sem enquadramento. A configuração versionada do firmware desabilita console, logs da aplicação e logs do bootloader nessa UART; diagnóstico operacional segue por `DaqcErrors` e pelo lado HOST. Debug do host não habilita prints indiscriminados do firmware no enlace. Controle DISABLE e confirmação precisam de caminho limitado mesmo sob carga. DATA e XRCE não podem bloquear o caminho de CONFIG.


### Supervisão de leitura e concorrência no firmware

F-27 usa 60 s sem avanço de confirmação cumulativa de leitura pelo host, mecanismo aprovado pelo usuário. Confirmações são emitidas pela thread de comunicação, nunca exigidas sincronicamente por doStep. Progresso é independente do valor lido: NaN lido também comprova leitura, embora alimente a proteção no host. Heartbeat simples não substitui confirmação de DATA.

A tarefa de supervisão deve usar relógio monotônico e espera por evento/prazo, sem busy loop. Parser atualiza progresso quando READ_ACK avança no STREAMING atual; supervisor não segura mutex durante I/O e solicita DISABLE por transição coordenada, sem competir com escritor dos atuadores. TX usa mailboxes/filas limitadas, sem espera que paralise RX/CONFIG. Frequência da supervisão e tolerância entre atingir 60 s e concluir DISABLE devem ser dimensionadas. Comparar carga/timing com e sem supervisão e sob saturação, incluindo interferência entre núcleos.

ESP-IDF oferece afinidade de tarefas entre os dois núcleos; a baseline do projeto é v5.2.6. Fonte: [FreeRTOS ESP-IDF v5.2](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/system/freertos.html). A inferência de design é separar responsabilidades; cumprimento temporal depende de medições.

A thread USB é distinta da thread de simulação. O snapshot usado é o da última atualização anterior à inserção FMI; timeout sem pacote não incrementa invalidade numérica. Proposta de contador: ausência preserva contagem; amostra válida zera, inválida incrementa uma vez por passo.

### Relógio e métricas de perdas — design da regra aprovada

Separar índice k da etapa FMI e índice j da liberação física. Para passo regular h, grade G(j)=T0+j*h; etapa FMI usa t(k)=t_start+k*h. Após terminar, executar próxima etapa na primeira liberação da grade não anterior ao término, sem sobreposição de doStep. Incrementar k somente por etapa concluída; liberações não usadas incrementam contador próprio. Caso de término exatamente na liberação não é perda dessa liberação. Validação do último passo parcial considera capacidades FMI e não deve alterar passos anteriores para compensar atraso.

Deadline da etapa liberada em G(j): D=G(j)+h. Atraso de conclusão=max(0,C−D), incluindo atraso de despertar. Registrar número de etapas entregues após D, número de instantes da grade não utilizados e maior atraso de conclusão, todos separados. Exemplo 0→12 ms, h=10 ms: uma entrega atrasada em 2 ms e uma liberação não usada em 10 ms; próximo doStep em 20 ms. Não somar esses dois contadores e chamar o resultado de etapas FMI descartadas. Se o próprio despertar perder uma liberação, medir o atraso e aplicar a mesma política de grade; não recuperar em rajadas.

Resultados temporais são apresentados após Finished/Stopped/Error, fora da thread crítica; somente contadores/relógio limitados no ciclo. Definir no teste qual fronteira é “entrega” (publicação host versus aplicação física confirmada); não apresentar enqueue USB como atuação observada. A duração configurada refere-se ao tempo do modelo; sem compensação, duração de parede pode aumentar.

## Reuso RaspDAQ — revisão 0.7

Base de serialização no ICD: little-endian, SYNC 59 72, CONFIG request/response 4/5 bytes e STATUS só em CONFIG. DATA carrega SEQ uint16 e schema posicional fixo N≤256, preservando MID 02 nos dois sentidos. Não há CRC, retransmissão ou sessão no fio. READ_ACK usa MID 03; XRCE usa MID 04 com entrega best effort no caminho periódico.

Reusar ordem de controle antes de DATA, rechecagem de estado no escritor, snapshots e sequência. Não copiar write_full bloqueante, locks durante I/O, prints incondicionais, defaults zero para campos ausentes ou reajuste de origem temporal do TX de referência. Controle reservado e mailbox de última atualização DATA preservam o fluxo real-time; perda detectada é contabilizada e o pacote novo segue.


### Revisão 0.25.7

O cliente micro-ROS usa chave inicial `0x4D48494C` e framing serial interno desabilitado: o único enquadramento UART é MID 04. A recuperação é limitada a uma tentativa por segundo em DISABLE ou ENABLE, nunca em STREAMING. A sessão física completa com o Agent continua pendente de evidência.
