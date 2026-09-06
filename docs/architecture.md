# Arquitetura de software

Revisão 0.2: código atual, decisões confirmadas e detalhes de design propostos. Implementação suspensa até concluir esclarecimentos, conforme usuário. Requisitos em [spec.md](spec.md); decisões não resolvidas em [decisions.md](decisions.md). Não há implementação dos módulos futuros apenas porque aparecem neste documento.

## ARCH-CORE — Núcleo, FMU e configuração

Preservar `src/fmu_model.c`, `src/rt_simulation.c`, `src/app_config.c` e headers como base. O wrapper já importa FMI 2.0 CS, enumera outputs e encapsula instância. Deve evoluir para validar inputs, tipos, unidades e mapeamento antes de executar. A capacidade de último passo variável precisa ser considerada: o exemplo permite, mas o loader aceita outros modelos. A meta é interpretar qualquer FMU 2.0 CS; Q-08 precisa definir tratamento de capacidades como Pending e tipos String. Não declarar suporte universal enquanto o wrapper os rejeita ou ignora. Formato compatível não comprova binário compatível com CPU nem custo computacional dentro do passo.

Qt e terminal debug desacoplados do núcleo foram confirmados. Proposta de implementação: separar a orquestração hoje contida em `main.c` em serviço testável, reutilizado por ambos. Um proprietário coordena inicialização, execução, parada e resultado final. Configuração é validada e congelada por execução; controles virtuais chegam por comando e são aplicados na fronteira de ciclo. Nenhum callback GUI/ROS chama a FMU diretamente.

| Módulo atual | Preservar | Ajustar/estender |
|---|---|---|
| app_config | Dados de execução | Validação finita, faixa, coerência e perfil |
| fmu_model | Importação, lifecycle, tipos de outputs | Inputs, capacidade/arquitetura, fixtures, falhas e diagnóstico limitado |
| rt_simulation | Thread dedicada, relógio monotônico absoluto | Estado, wakeup de stop, métricas completas, entradas/saídas consistentes |
| main | Fluxo CLI útil para diagnóstico | Extrair lógica compartilhável com GUI e propagação de falha |
| sample_queue | Cópia SPSC e atomics | Testes concorrentes, limites e política por consumidor |
| csv_logger | Consumidor assíncrono | Falhas/encerramento como reuso; novo sink binário tipado e CSV apenas pós-execução |
| plotter | Prova de desacoplamento | Instrumento transitório até Qt; não satisfaz janelas individuais/controle ao vivo |

## ARCH-STATE — Comandos, estados e falhas

Estados host e estados DAQC são máquinas distintas. Host preserva Idle/Running/Error/Stopped/Finished. DAQC tem DISABLE, ENABLE (IDLE) e STREAMING conforme DEC-012. Não confundir ENABLE/IDLE físico com tela Idle.

| Evento | Comportamento confirmado | Detalhe ainda aberto |
|---|---|---|
| Carregar FMU | Botão/diálogo próprio; validar FMI 2.0 CS e analisar nomes/tipos | Capacidades suportadas, Q-08 |
| Carregar configuração | Outro botão; comparar FMU/DAQ/mapa e parâmetros, listar divergências | Identidade/versão no arquivo, Q-07/Q-09 |
| Play | Inicializar pela FMU/start e executar com mapa válido | Saída calculada/ausência de start e DAQC antes de configurar, Q-04 |
| Running + deadline perdido | Continuar, contar e guardar pior atraso | Agenda posterior, Q-06 |
| Stop/fim normal/erro | Último valor válido permanece como referência de saída; indicar motivo e permitir novo run | Aplicação elétrica sob link perdido/reset, Q-09 |
| NaN/Inf/outro inválido | Não enviar valor inválido; usar último aceitável | Definição de faixa após conversão e ausência de qualquer aceitável, Q-04/Q-05 |
| Mais de 100 inválidos consecutivos + checkbox | Acionar proteção e mensagem GUI no 101º inválido | Pausa automática versus encerramento Error, contador por saída/global e retomada, Q-03 |
| Fechar aplicação normalmente | DAQC deve aceitar DISABLE mesmo em STREAMING e liberar fluxo | Prazo/ack e falha na confirmação, Q-09 |
| Processo morto/reset da DAQC | Não foi definido completamente | Não há garantia de envio de DISABLE por processo morto; retenção após reset depende de política, Q-04/Q-09 |

Não há botão de pausa manual requerido. O estado Paused não é inventado até Q-03 resolver a pausa automática solicitada. Ao repetir a simulação, reinicializar lifecycle/configuração e não herdar contadores e resultados sem contrato explícito.

Proposta: manter por saída último valor aceitável e qualidade, separados do valor bruto FMU e do último valor confirmado/aplicado no dispositivo. Uma chamada de escrita bem-sucedida no host não prova atuação física. O escritor final do atuador deve ser único, inclusive na aplicação do valor retido.

Diagnósticos próprios e callbacks de erro da FMU são encaminhados à GUI fora do caminho crítico. Terminal só em debug, também por consumidor não crítico; nenhum printf por ciclo. Código legado contém stderr e ainda será corrigido. Mensagens emitidas diretamente por binário externo não são automaticamente controladas pelo flag do aplicativo.

Observação de arquivo: a FMU atual tem 14 outputs e 12 não possuem start explícito no XML (inspeção sem execução). Ler start literal não é suficiente para inicializar todos os modelos. O [schema FMI 2.0.4](https://raw.githubusercontent.com/modelica/fmi-standard/v2.0.4/schema/fmi2ScalarVariable.xsd) distingue inicialização exact/approx/calculated. Proposta: usar valor pós-inicialização quando calculado, sujeito a Q-04.

## ARCH-TIME — Ciclo e orçamento temporal

Meta confirmada: operar pelo menos a 100 Hz para modelos cujo custo caiba no alvo, sem fixar planta; passo e duração vêm da GUI/terminal. 100 Hz corresponde a 10 ms. Não prometer 100 Hz para toda FMU importável. Ter fixtures de benchmark para ensaio não restringe o produto a uma planta.

Fluxo de design: snapshot consistente → aplicar inputs → um doStep → ler/validar outputs → selecionar último valor aceitável → publicar saída do ciclo → enfileirar uma amostra final por passo → agendamento. Leituras assíncronas/snapshots devem evitar duas esperas sequenciais de comunicação no orçamento; sincronização e validade ainda precisam de Q-06. Saída bruta versus aceita no log ainda Q-07.

Definições propostas a confirmar: agenda ideal `deadline_n = t0 + (n+1)*h`; atraso de entrega `max(0, entrega_n - deadline_n)` e contagem de ciclos com atraso. Medir também duração de trabalho e atraso de despertar, pois uma tarefa que trabalhou menos de h pode ter sido liberada tarde. Q-06 define se a agenda é mantida para recuperação ou deslocada após perdas. Não pular etapas da FMU silenciosamente.

| Item | Confirmado ou pendente |
|---|---|
| Passo e duração | Configuráveis; positivos/finitos, coerentes com capacidades da FMU |
| Referência de desempenho | 100 Hz / 10 ms; não é passo fixo obrigatório |
| Overrun | Continuar; exibir contagem e pior perda, além das estatísticas F-13 |
| Timeout USB | Máximo 5 ms; Q-06 define por transferência ou troca completa. API configurada não prova limite real sob scheduler |
| Renderização | Até 10 Hz, desacoplada; frames gráficos podem ser coalescidos sem descartar amostras do log |
| SCHED_FIFO | Continua requisito; permitir fallback de diagnóstico ainda Q-06 |
| Idade de entrada/cadência de aquisição | Não derivar do timeout USB nem do teto gráfico; Q-06 |
| Jitter aceitável/duração/carga de ensaio | Detalhar na verificação; limites não vieram do Demo |

Duas esperas sequenciais de 5 ms já consomem 10 ms antes de FMU/filas/overhead. Para serial 8N1, se selecionada, 259 bytes em 115200 baud exigem aproximadamente 22,48 ms apenas no fio. É exemplo aritmético, não baud rate adotado ou medição. Maximizar taxa significa medir o maior ajuste estável do caminho completo, não usar taxa nominal USB como prova.

Coletar médias/máximos de ciclo, FMU e leitura/escrita, contagem de timeouts e pior atraso para apresentação final. Esses agregados ficam separados do stream binário de saídas. Escopo exato de persistência de metadados está em IF-LOG. Não fazer I/O textual/disco/GUI/ROS síncrono no ciclo, nem alocar snapshots Python na thread crítica C por analogia com RaspDAQ.

## ARCH-IO — Comunicação e perfis

Bulk/libusb e micro-ROS no ESP32 permanecem confirmados. A USB-C da placa liga host → CH340 → UART do ESP32; não é uma interface nativa USB de ESP32-S3. Libusb pode ser parte de acesso direto à ponte, mas precisa de controle/configuração do dispositivo, exclusividade frente ao driver e integração XRCE explicitamente desenhada. Nenhuma dessas partes está implementada.

RaspDAQ foi descrito como serviço com USB/nó ROS e snapshots sob mutex no mesmo processo. Não localizado por busca de nomes nas pastas Projects/Downloads; código não revisado. Esse padrão pode evitar IPC para o estado do serviço, mas não demonstra que o nó ROS substitua o agente exigido pelo cliente micro-ROS no ESP32.

Q-01 define agente/transporte e multiplexação ou canal separado para XRCE. O protocolo CONFIG/DATA sozinho transporta I/O, não torna micro-ROS interoperável. Não deixar libusb e agente TTY lerem concorrentemente a mesma porta. NF-06 continua C/libusb até decidir papel de Python; possibilidade de wrapper Python não revoga a camada C por inferência.

Proposta: dono único do enlace, demultiplexação no adaptador, snapshots completos com geração publicados sob lock limitado, núcleo só copia estruturas internas; ninguém mantém mutex durante I/O/ROS. Double-buffering exigido por NF-08 continua base a reconciliar com objeto imutável do serviço. Recriar objeto no serviço não requer recriá-lo por passo na thread C.

Perfil ESP32 é catálogo de recursos com exclusões de função, não soma de todas as capacidades simultâneas. No mapa, DI/AI da DAQC alimentam inputs da FMU; outputs FMU destinados ao hardware alimentam DO/AO. Unidades e faixas precisam de conversão explícita: real da FMU não significa volts por definição. ICD em [interfaces.md](contracts/interfaces.md).

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

Log binário de saídas finais por passo; conversão CSV após encerramento, utilizando FMU para interpretar tipos. Não converter durante Running. Configuração também permanece binária. IF-LOG trata identidade/ordem, layout e política de falha; métricas por passo saem do log por instrução do usuário.

## ARCH-LOG — Dados e concorrência

| Dado | Proprietário proposto | Acesso |
|---|---|---|
| Configuração validada e mapa | Controlador da execução | Imutável durante run; cópia/configuração versionada |
| Instância e tempo FMU | Thread de simulação | Chamadas exclusivas durante run; lifecycle serializado |
| Input snapshot | Adaptador de entrada | Snapshot completo, geração, sequência e qualidade; mecanismo conforme NF-08/DEC-001 |
| Comandos virtuais | Controlador aceita; simulação aplica | Publicação limitada e confirmação de aplicação na fronteira de ciclo |
| Output snapshot | Simulação | Cópias para adaptador de saída/telemetria; sem ponteiros mutáveis para widgets |
| Filas log/plot | Simulação produz, um consumidor por fila | SPSC enquanto topologia for exatamente essa; não adicionar consumidor à mesma fila sem revisão |
| Estado público | Controlador da execução | Eventos ordenados e snapshots; erro dos workers propagado |
| Retenção/estado de saída físico | Supervisor local DAQC | Atuador com escritor final único; não competir PWM/DAC entre tarefas |

Históricos gráficos são limitados à janela e à vida do gráfico; renderizações podem ser coalescidas. Fila de log implica integridade de registro e exige Q-07 para reação à falha. Essa diferença precisa de contrato, sem espera ilimitada na simulação. Encerramento deve drenar após observar fim de produção e verificar novamente a fila; erro de escrita/close deve chegar ao resultado agregado.

No HOST auditado, cada fila ocupa 2.228.248 bytes e há duas na pilha de main. O histórico de plot tem 5000 amostras. Dimensionar buffers, pilhas e atomics no alvo, com limites que ainda não existem; não inferir orçamento do firmware a partir dessa alocação Linux.

## ARCH-FW — Firmware proposto

Firmware ainda ausente; não será implementado antes de fechar os Markdown. ADC/DAC internos confirmados, perfil ESP32 e capacidades do TARGET. Estados obrigatórios DISABLE, ENABLE (IDLE), STREAMING; parser atende CONFIG em todos eles e não fica preso em transmissão contínua. DISABLE deve interromper streaming sem matar a capacidade de receber futuros comandos.

Separar aquisição/aplicação, parser, controle de estado, diagnóstico e integração micro-ROS. Tarefas, prioridades, clocks, RTOS e versões serão definidos com o contrato fechado; não copiar ESP32-S3 do Demo. Atualização de saída tem escritor final único e retém último valor aceitável nas condições já definidas. Reset sem último valor e estado elétrico antes da configuração continuam Q-04; desconexão/fechamento abrupto Q-09.

UART0 usada para dados não pode misturar logs de debug sem enquadramento. Debug do host não habilita prints indiscriminados do firmware no enlace. Controle DISABLE e confirmação precisam de caminho limitado mesmo sob carga; prazo e semântica de confirmação são parte do ICD pendente.

