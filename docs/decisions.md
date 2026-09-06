# Decisões e esclarecimentos da especificação

Revisão 0.6. Respostas DEC-001…012 e Q-01…09 incorporadas. **Implementação suspensa por instrução do usuário até consolidar os Markdown e sanar as lacunas.** Correção mais recente e respostas complementares prevalecem: retenção nos inputs FMU do host, 100 passos com checkbox, confirmação cumulativa e 60 s sem leitura para DISABLE. O histórico de revisões anteriores permanece nas evidências; este documento registra o estado vigente. Detalhes ainda abertos não anulam respostas recebidas.

## Decisões de produto

| ID | Confirmado | Detalhamento restante |
|---|---|---|
| DEC-001 | Bulk/libusb e micro-ROS no ESP32, USB-C da placa atual; reuso do padrão de estado compartilhado do RaspDAQ | Q-01: transporte XRCE e dono do enlace; Python continua alternativa |
| DEC-002 | Qt 6/C++ desacoplado, terminal debug, prioridade GUI inferior ao núcleo, aparência Mint; binário e CSV posterior | Q-07: layout de arquivo e representação de inválidos |
| DEC-003 | Perfil ESP32, recursos selecionáveis com exclusão por GPIO e reserva UART; ADC/DAC internos e PWM | Q-05: faixa ADC e configuração PWM |
| DEC-004 | FMU 2.0 CS sem planta fixa, passo/duração configuráveis; meta 100 Hz medida por modelo/alvo; continuar após deadline, contar perdas e pior atraso; USB ≤5 ms por transferência | Q-06: agenda após atraso e identificação de pacote tardio |
| DEC-005 | Aquisição inválida retida no host, inicial válido do input se necessário; checkbox/100 passos. Encerramento da simulação zera saídas físicas e cessa DATA; restart usa outputs iniciais da FMU reinicializada | Entrega/ordem de encerramento no ICD |
| DEC-006 | Primeiro Linux Ubuntu 22.04/ROS 2 Humble; futuro Raspberry Pi 4/2 GB | Versões reproduzíveis, kernel/arquitetura do Pi e firmware a detalhar no TARGET |
| DEC-007 | Continuar após overrun; último dado válido pode ser reutilizado até fim da execução; saída constante não é defeito | SCHED_FIFO precisa ser confirmado pelo SO, não presumido; agenda em Q-06 |
| DEC-008 | Saída da FMU por passo no binário, sem confundir com o input DAQC retido no host; inválidos representados por NaN ou zero; falha de gravação não interrompe simulação, mas gera aviso | Q-07: representação por tipo e metadados |
| DEC-009 | Sem pausa; gráfico até 10 Hz, ticks Y configuráveis, janela temporal comum deslizante; fechar destrói histórico, reabrir começa dali; configuração e abertura separadas, abertura antes de Play permitida | Sem conflito pendente com proteção: ela termina em Error |
| DEC-010 | Preservar APIs Qt; convenção própria C++ e estilo definidos na constituição | Nenhuma decisão de toolkit reaberta |
| DEC-011 | Importação de FMU separada da configuração binária; diagnosticar incompatibilidades específicas com FMU/DAQ/mapa | Pode identificar e informar capacidades não suportadas no incremento inicial (Q-08 resolvida) |
| DEC-012 | SYNC 0x7259, MID CONFIG=01/DATA=02, COMMAND DISABLE=01/ENABLE=02/STREAMING=03; payload até 256 bytes; STATUS só ESP32→host usa códigos de COMMAND; CONFIG atendido em streaming | Base de bytes/STATUS/comprimento definida por ADR-003; Q-01/Q-06/Q-09: extensões e validação |

## Respostas Q incorporadas

| ID | Resposta recebida e efeito | Situação |
|---|---|---|
| Q-01 | Referência disponível em Projects/OT1-HiLInfrastructure; raspdaq_main, raspdaq_ffs, raspdaq_node e shared_daq_state inspecionados estaticamente | Localização resolvida; arquitetura XRCE ainda aberta |
| Q-02 | Consolidado com o serviço RaspDAQ: little-endian, SYNC 59 72; CONFIG 4/5 bytes, STATUS uint8 só no retorno CONFIG; DATA base com SEQ e N fixo por direção | Base definida em ADR-003; não repetir pergunta de ordem/tamanho/STATUS. Extensões têm status próprio |
| Q-03 | Manter checkbox. 100 passos consecutivos inválidos por canal adquirido → Error e novo Play, identificando todos os canais/input FMU que atingiram limite | Resolvida; substitui limite 101 e escopo de saída da revisão 0.3 |
| Q-04 | Retenção no host; sem histórico DAQC válido, usar valor inicial válido da entrada FMU. Ausência de start literal não impede inicialização calculada | Resolvida; não inferir zero nos atuadores ou no input sem referência válida |
| Q-05 | Recursos com reservas/exclusões atendem; incluir PWM; mapa em volts, menção incerta a 0…255 | Inclusão/unidade analógica resolvidas; 0…255 é código DAC, parâmetros ADC/PWM abertos |
| Q-06 | ≤5 ms por transferência; descartar atrasado, usar último aceitável no passo até fim; não detectar falha por valor constante | Resolvido comportamento de dados; faltam agenda e mecanismo de identificação no fio |
| Q-07 | Continuar com aviso em falha de log; registrar saída FMU, inválido NaN ou zero; usar identidade fornecida pelo modelo | Política resolvida; falta formalizar tipos/arquivo |
| Q-08 | Identificar e informar capacidades não suportadas | Resolvida; não declarar suporte universal nem restringir silenciosamente |
| Q-09 | Em STREAMING, 60 s sem progresso de leitura → DISABLE, sem zeramento. Confirmar cumulativamente último pacote lido pelo host, na thread de comunicação; usar os dois núcleos do ESP32 | Limiar e mecanismo aprovados; layout/cadência/tolerância/sessão ainda abertos |

## Esclarecimentos técnicos

**RaspDAQ:** o processo cria uma instância duradoura de SharedDaqState e a entrega ao runtime USB e nó rclpy. Os snapshots imutáveis são substituídos sob RLock; o nó não é recriado por snapshot. FunctionFS fornece o lado dispositivo USB em Linux. A placa ESP32 informada usa CH340/UART; o padrão de estado pode ser aproveitado, mas endpoints FunctionFS não são um driver ESP32. O protocolo RaspDAQ usa outros SYNC, comandos e comprimentos; não substitui o ICD MICROHIL. [Evidência de inspeção](evidence/q-review-2026-09-06.md).

**Volts e códigos:** 0…255 são os 256 códigos do DAC de 8 bits. A referência ideal é Vout ≈ código × VDD3P3_RTC / 255. O mapa analógico permanece em volts; a conversão para código é explícita e a tensão útil depende de caracterização. PWM é duty cycle/frequência; não é tensão analógica contínua sem circuito apropriado. [Fonte DAC](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/dac.html).

**IDs da FMU:** valueReference serve às chamadas FMI e não é necessariamente único entre todas as variáveis. Não significa posição sequencial do arquivo. O contrato propõe usar a ordem XML das saídas selecionadas e guardar índice, nome, tipo e valueReference associados à FMU. [Schema oficial](https://raw.githubusercontent.com/modelica/fmi-standard/v2.0.4/schema/fmi2ScalarVariable.xsd).

**SCHED_FIFO:** continua obrigatório. Configurar a política pode falhar, por exemplo por privilégios insuficientes (EPERM); a aplicação precisa verificar retorno e política efetiva. Proposta: impedir Play HiL e informar a causa se falhar, sem fallback silencioso. Isso não conflita com continuar após deadline perdido durante uma execução já iniciada. [Linux man-pages](https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html).

**Constância e comunicação:** um valor constante válido não incrementa inválidos e sua leitura comprova progresso. A proteção de aquisição conta passos no host; DISABLE conta 60 s sem progresso de confirmação de leitura no firmware. Um dado NaN efetivamente lido avança a confirmação, mas participa da contagem de invalidade no host. Essa separação evita desabilitar a DAQC apenas porque a medição é constante ou numericamente inválida.

## Lacunas restantes, sem repetir decisões resolvidas

| ID | Decisão necessária | Proposta para revisão, ainda não aprovada |
|---|---|---|
| Q-01 | RaspDAQ tem rclpy/FunctionFS; não tem transporte micro-ROS ESP32 | E-XRCE propõe coordenador/dono único e callbacks no agente e cliente; sem atribuir implementação à referência |
| Q-02 | Consolidado com o serviço RaspDAQ: little-endian, SYNC 59 72; CONFIG 4/5 bytes, STATUS uint8 só no retorno CONFIG; DATA base com SEQ e N fixo por direção | Base definida em ADR-003; não repetir pergunta de ordem/tamanho/STATUS. Extensões têm status próprio |
| Q-05 | Configuração ADC/PWM pelo usuário autorizada | Opções/limites explicitados no TARGET; validar combinações e recursos antes de Play |
| Q-06 | Última atualização na inserção FMI; leitor em thread própria; ausência separada. Após atraso, aguardar próximo instante fixo mantendo passo/sequência FMI | Comportamento confirmado; detalhar correlação no ICD e SCHED_FIFO negado |
| Q-07 | Não há formato binário MICROHIL na referência inspecionada | IF-LOG apresenta design host tipado/versionado; CFG depende do schema integrado. Não copiar dumps/prints como log |
| Q-09 | Layout/cadência da confirmação cumulativa, sessão, tolerância de supervisão e buffers | Leitura de DATA completo renova progresso mesmo com NaN; ACK duplicado/antigo e heartbeat sem leitura não renovam. 60 s confirmado, sem zeramento e sem armazenamento ilimitado |

Respostas 1…5 e esclarecimento de grade fixa foram incorporados. Não reabrir seleção por passo, ausência separada, configuração ADC/PWM, zero físico no fim, restart pela FMU ou política temporal. Restam detalhamentos do ICD, versões e critérios de ensaio; não são novas perguntas sobre escolhas já confirmadas.
## Correção de direção e uso dos núcleos

Mundo real → AI/DI DAQC → USB → host → input FMU é aquisição; output FMU → USB → AO/DO/PWM DAQC → mundo real é atuação. “Saída da DAQC” na GUI não deve designar ambiguamente o canal de aquisição com erro: informar canal físico adquirido e input FMU mapeado. A redação 0.3 que mandava zerar atuadores por ausência de host foi substituída, não mantida como política adicional.

Aprovado usar os dois núcleos ESP32 em benefício do timing. Proposta de distribuição: aquisição/atuação em um núcleo; comunicação, micro-ROS e supervisão no outro. Afinidades/IRQs/prioridades exatas serão dimensionadas e medidas; não alegar ausência absoluta de interferência por divisão de núcleos.

## Respostas 1…5 incorporadas — revisão 0.5

1. Sem novo pacote: reutilizar referência válida e registrar timeout separadamente; não incrementar invalidade numérica.
2. Vários pacotes: usar somente última atualização disponível antes da inserção na FMU. Leitor USB e simulação obrigatoriamente em threads distintas.
3. Atuação de dados somente STREAMING. Ao acabar a execução, zero físico e cessação de envio. Novo Play processa novamente inicialização da FMU e seus outputs iniciais. O zero do encerramento não é causado por uma amostra inválida isolada.
4. ADC e PWM configuráveis; opções e restrições explicitadas no TARGET a partir de fontes Espressif.
5. Confirmado pelo usuário: após cálculo 0→12 ms com passo 10 ms, próxima etapa começa em 20 ms, na grade fixa. Não saltar etapas da FMU, não executar rajadas e não alterar h. Mostrar perdas de deadline, instantes de execução perdidos e worst case após a simulação.

## Resultado da consulta RaspDAQ — revisão 0.6

Não há novas perguntas sobre o que o código já resolve. Base CONFIG/DATA, mapa posicional e ownership consolidados no ICD. Quatro ausências relevantes foram identificadas: sessão/ACK cumulativo, integridade/framing robusto UART, transporte micro-ROS/CH340 e arquivos binários MICROHIL. O ICD propõe adaptações técnicas separadas para essas lacunas, sem chamar propostas de recursos encontrados. [ADR-003](adrs/ADR-003-raspdaq-icd.md) contém resposta item a item e diferenças entre serviço e CLI de Sandbox.
