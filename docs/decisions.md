# Decisões e esclarecimentos da especificação

Revisão 0.2. Respostas DEC-001…012 incorporadas. **Implementação suspensa por instrução explícita do usuário até sanear as dúvidas e consolidar os Markdown.** Detalhamento pendente não desfaz escolhas confirmadas. Não voltar a perguntar Qt, formato principal, nome do perfil ou modelo do Pi.

## Respostas registradas

| ID | Confirmado pelo usuário | Ainda necessário |
|---|---|---|
| DEC-001 | Bulk/libusb e micro-ROS no ESP32, USB-C da placa, inspiração RaspDAQ com serviço USB/nó ROS e snapshots sob mutex no mesmo processo | Código RaspDAQ, transporte/agente XRCE, coexistência com frames próprios; Python foi possibilidade, não substituição de C/libusb. Q-01 |
| DEC-002 | Qt 6 desacoplado, terminal debug, GUI baixa prioridade, aparência Mint; log binário durante execução e CSV somente depois, tipos interpretados com FMU | Metadados/ordem do arquivo e falha de gravação. Q-07 |
| DEC-003 | Perfil ESP32 com todos os I/Os disponíveis e ADC/DAC internos; faixas nativas | Recursos multiplexados, reservas, atenuação ADC, unidade e inclusão de PWM/periféricos. Q-05 |
| DEC-004 | Sem planta fixa, FMU 2.0 CS, nomes/tipos interpretados, passo/duração configuráveis; continuar após deadline; contagem e pior atraso; timeout USB máximo 5 ms; meta 100 Hz | Escopo do timeout, agenda após atraso e aceitação por capacidade FMU. Q-06/Q-08 |
| DEC-005 | Start do modelo na inicialização; reter último válido em Stop/fim/erro/NaN/Inf; checkbox para mais de 100 inválidos consecutivos; mensagem GUI, prints só debug, repetir run | Boot sem modelo, start calculado, contador, pausa automática versus Error, reset/link. Q-03/Q-04/Q-09 |
| DEC-006 | Raspberry Pi 4 de 2 GB posteriormente; primeiro Linux Ubuntu 22.04 com ROS 2 Humble; micro-ROS compatível | SDK/toolchain/versões exatas, arquitetura do SO/kernel do Pi |
| DEC-007 | Overrun resolvido em DEC-004: continuar e medir | Prioridade negada, idade de entrada e agenda. Q-06 |
| DEC-008 | Somente saídas finais por passo em binário | Disco/fila cheia, saída bruta versus aceita e metadados. Q-07 |
| DEC-009 | Sem pausa manual; resolução é espaçamento Y; janela temporal comum deslizante; até 10 Hz; fechar destrói histórico; reabrir coleta do instante atual; abrir antes de Play; controles de configuração/abertura distintos | Apenas conflito com pausa automática de DEC-005. Q-03 |
| DEC-010 | Preservar APIs Qt e definir estilo próprio consistente | Convenção C++ de design registrada na constituição; não há necessidade de renomear APIs Qt |
| DEC-011 | Botão/dialog de FMU separado de configuração binária; conferir FMU 2.0 CS, configuração versus FMU/DAQ e divergências específicas | Tipos String/Enumeration, Pending, start ausente, plataformas binárias; fixtures não são planta obrigatória. Q-04/Q-08 |
| DEC-012 | SYNC 0x7259; MID CONFIG=01/DATA=02; COMMAND DISABLE=01/ENABLE=02/STREAMING=03; até 256 bytes de payload; STATUS só ESP32→host; CONFIG funciona em streaming, DISABLE libera enlace | STATUS repetir MID não distingue estados; bytes/tamanho/fronteira, STATUS em DATA, sessão/perfil/integridade. Q-02/Q-09 |

Registro transversal: [ADR-002](adrs/ADR-002-product-decisions.md). Protocolo: [ICD](contracts/interfaces.md). Fontes originais preservadas.

## Explicação de DEC-007

A 100 Hz, um período é 10 ms. Se a saída prevista para 10 ms fica disponível em 12 ms, houve atraso de 2 ms. Você já decidiu continuar e contabilizar. Resta escolher se os próximos ciclos tentam recuperar a agenda absoluta sem pular passos, ou deslocam a agenda para não executar rajadas, aceitando desvio crescente entre tempo simulado e tempo real. Uma FMU computacionalmente lenta não se torna capaz de 100 Hz por ser importável.

As outras perguntas eram: se Linux negar SCHED_FIFO, rejeitar HiL ou permitir apenas diagnóstico sem garantia temporal; e por quanto tempo reutilizar a última entrada DAQC se não chegar nova amostra. Timeout de transferência e idade da amostra são medidas diferentes.

## Explicação de DEC-008

O conteúdo está definido: saída final por passo, sem registrar cada subpasso interno. A dúvida é a reação se disco/fila não acompanhar. Continuar requer declarar log incompleto; parar requer encerramento e retenção das saídas. A thread de simulação não pode esperar indefinidamente por disco e não pode declarar preservação de dados que não foram gravados.

Sua resposta retira métricas de desempenho do stream de saídas (F-10/12), mas mantém estatísticas finais em F-13/NF-28. Metadados de identidade, ordem das variáveis e referência temporal precisam ser definidos para interpretar o arquivo; isso não implica registrar um stream adicional de métricas.

## Perguntas restantes

Pode responder pelos IDs Q. Nenhuma sugestão abaixo foi promovida a decisão física ou de protocolo.

| ID | Pergunta | Observação/proposta |
|---|---|---|
| Q-01 | Onde está o RaspDAQ? Micro-ROS terá outro transporte ou deve ser multiplexado na mesma USB–UART? Python é obrigatório ou só alternativa? | Nó ROS que publica snapshots não substitui agente XRCE. Um canal precisa de dono único e enquadramento compatível |
| Q-02 | STATUS repete COMMAND (01/02/03), não MID (01/02)? Tem um byte e aparece também no DATA de retorno? SYNC no fio será 72 59? Payload tem tamanho fixo por perfil/direção ou campo de comprimento? | SYNC pode ocorrer dentro de dados; só procurar o próximo SYNC não resolve framing |
| Q-03 | Após 101 inválidos: pausa automática recuperável sem pausa manual ou Error e novo Play? Contagem por saída ou global? | Limite >100 e checkbox já confirmados. Proposta de contador por saída, zerado por valor válido; retomada ainda precisa de regra |
| Q-04 | Antes de receber modelo/valor válido, qual estado elétrico no boot/reset? Se output não tem start literal, usar valor após inicialização FMI? Se continuar inválido, bloquear Play? | Não existe último valor válido garantido no primeiro boot. A DAQC não conhece a FMU que ainda será carregada |
| Q-05 | Todos os recursos selecionáveis, uma função por GPIO, reservando UART0 e tratando pinos de boot, atende “todos os I/Os”? Qual atenuação ADC e representação: volts/código bruto? Primeiro perfil inclui PWM/periféricos ou DI/DO/AI/AO? | ADC/DAC internos e nome ESP32 já decididos. Não é possível somar canais analógicos e digitais conflitantes como independentes |
| Q-06 | Os 5 ms valem por transferência ou troca completa? Recuperar agenda absoluta ou deslocá-la após atraso? Se SCHED_FIFO falhar, permitir só debug? Qual idade máxima de entrada? | 100 Hz dá 10 ms ao ciclo inteiro. Duas esperas sequenciais de 5 ms consomem esse orçamento antes de calcular a FMU |
| Q-07 | Se gravar falhar, continuar com aviso/log incompleto ou Error? Gravar saída bruta ou valor aceito após filtro? Cabeçalho pode identificar FMU, ordem de saídas, passo e referência temporal? | Proposta: valor aceito + metadados mínimos. FMU sozinha não informa seleção/ordem do log. Política de queda de energia não foi solicitada como garantia e não será inventada |
| Q-08 | A meta qualquer FMU inclui String e execução assíncrona Pending, ou primeiro incremento reconhece e diagnostica capacidades não suportadas? | Formato, capacidades de execução e 100 Hz são verificações diferentes; não excluir silenciosamente capacidades |
| Q-09 | Em perda de link/fechamento abrupto, manter valor enquanto alimentada? Fechamento normal aguarda confirmação DISABLE? Perfil/sessão serão conferidos por handshake ou contrato fixo de firmware? Detectar corrupção por CRC? | DISABLE durante streaming já é obrigatório. Host morto não envia comando; reset da DAQC é diferente de desconexão. SYNC não é checksum ou identidade de sessão |

Não iniciar implementação, mesmo HOST independente, antes da consolidação solicitada. Trabalho documental e consultas técnicas continuam sem depender de respostas inexistentes.
