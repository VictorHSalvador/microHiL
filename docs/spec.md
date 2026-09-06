# Especificação de trabalho do MICROHIL

Referência de origem: MICROHIL-REQ-001-A, versão 01 de 24.08.2026. Revisão de trabalho: 0.2, 06.09.2026. **Estado: em elaboração, com decisões técnicas pendentes.** Este é o documento editável para a retomada solicitada; não é uma revisão A retroativamente alterada nem declaração de validação.

## Origem e regra de leitura

Os 53 IDs de origem foram preservados. Respostas DEC-001…012 atualizam os textos afetados nesta revisão, conforme [ADR-002](adrs/ADR-002-product-decisions.md). Cinco requisitos derivados foram acrescentados: F-23…26 e NF-32, totalizando 58. A [referência A](references/MICROHIL-REQ-001-A.md) continua intacta; estilo C/Python foi confirmado na [ADR-001](adrs/ADR-001-coding-style.md).

**Texto vigente:** exigência recebida ou alteração confirmada; detalhes ambíguos não foram inventados. **Critério de aceitação proposto:** refinamento para teste, não evidência executada. **Pendência:** DEC/Q identifica o que falta fechar, não anula respostas já dadas. A implementação permanece suspensa por solicitação do usuário até consolidar os Markdown.

Confirmados: Qt 6 desacoplado/terminal debug, Bulk/libusb + micro-ROS com arquitetura a detalhar, perfil ESP32/ADC-DAC internos, Pi 4/2 GB após Linux inicial, logging binário de saídas/CSV posterior, retenção de último válido, meta 100 Hz/timeout USB até 5 ms, continuar após overrun e gráficos até 10 Hz. Perguntas restantes em [decisions.md](decisions.md).

Comentários de todo código próprio devem estar em inglês, ser breves e úteis, conforme [constituição](constitution.md). Não quebrar parâmetros por estética; quebrar apenas linhas extremamente longas. Essas regras são instruções confirmadas do usuário, aplicáveis a todos os incrementos, sem criar funcionalidades ou mecanismos de hardware adicionais.

## Requisitos funcionais

### REQ-F-01 — Carregamento de FMU

A plataforma deve permitir importar modelos FMU 2.0 Co-Simulation sem depender de uma planta fixa, por botão e diálogo próprios na GUI, identificar nomes e tipos de entradas/saídas e verificar compatibilidade com o sistema antes de executar. Capacidades e binários incompatíveis devem ser diagnosticados especificamente, sem alegar suporte que não foi implementado.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-01; HOST):** Importar fixtures de diferentes modelos/tipos; identificar FMI/kind, nomes, tipos e arquitetura. Rejeitar arquivo incompatível com causa explícita. Q-08 fecha capacidades especiais, sem restringir silenciosamente o produto à FMU de exemplo.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-003. **Pendência:** DEC-011; Q-08.

### REQ-F-02 — Configuração de Entradas e Saídas

A plataforma deve permitir associar inputs/outputs da FMU aos recursos do perfil DAQC selecionado, iniciando pelo perfil ESP32. O mapa deve respeitar tipo, direção, unidade/faixa e exclusividade de funções dos pinos; capacidades multiplexadas não são canais simultâneos independentes.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-02; HOST + bancada/HIL):** Mapear canais compatíveis e rejeitar funções conflitantes, direção/tipo/unidade incompatíveis; conferir o perfil ESP32 e diagnosticar o ponto de divergência ao carregar configuração.

**Design:** ARCH-IO / IF-CORE. **Execução:** TASK-006. **Pendência:** DEC-003/011; Q-05.

### REQ-F-03 — Seleção e Controle da DAQC

A plataforma deve permitir selecionar perfil DAQC compatível, inicialmente denominado ESP32, e habilitar/desabilitar sua participação na simulação. Os estados da DAQC são DISABLE, ENABLE (IDLE) e STREAMING, com confirmação de estado no protocolo a detalhar.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-03; HOST + HIL):** Selecionar ESP32, operar sem DAQC quando desabilitada e verificar transições/confirmação quando habilitada; DISABLE recebido em STREAMING interrompe fluxo sem bloquear comandos posteriores.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-006, TASK-008. **Pendência:** DEC-003/012; Q-02/Q-09.

### REQ-F-04 — Persistência do Perfil de Configuração

A interface deve salvar/carregar configuração em arquivo binário, incluindo I/O, mapa FMU–DAQC e parâmetros de execução. Carregamento usa botão distinto do de FMU e valida o arquivo contra a FMU carregada e a DAQC selecionada, informando quais variáveis, tipos, recursos ou parâmetros divergem antes de aplicar a configuração.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-04; HOST + GUI):** Round-trip completo; arquivos incompatíveis/corrompidos não aplicam configuração parcial e apresentam divergência por campo. Abrir configuração não substitui silenciosamente a FMU ou DAQC selecionada.

**Design:** ARCH-GUI / IF-LOG. **Execução:** TASK-009. **Pendência:** DEC-002/011; Q-07/Q-09.

### REQ-F-05 — Configuração da Execução da Simulação

GUI e terminal debug devem permitir definir passo e duração da simulação, mantendo coerência e validando valores finitos positivos e compatibilidade com capacidades da FMU. O produto não exige uma planta ou passo fixos.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-05; HOST + GUI):** NaN/Inf/zero/negativo/overflow rejeitados; passo e duração configurados são usados. Verificar último passo parcial e capacidades da FMU sem fixar o modelo de exemplo como única planta.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-003, TASK-009. **Pendência:** DEC-004/011; Q-06/Q-08.

### REQ-F-06 — Início da Simulação

A plataforma deve iniciar a execução da simulação quando o usuário acionar o comando Play, desde que as configurações necessárias estejam válidas.

**Critério de aceitação proposto (V-F-06; HOST + GUI):** Play válido inicia uma única execução; Play inválido não executa passo nem habilita atuação; chamadas repetidas não criam runners concorrentes.

**Design:** ARCH-STATE / IF-CORE. **Execução:** TASK-003, TASK-009. **Pendência:** DEC-005.

### REQ-F-07 — Parada da Simulação

Stop deve interromper a execução de modo consistente, finalizar os registros até o último ciclo concluído quando o destino estiver disponível e manter nas saídas a referência do último valor válido. Falha de armazenamento ou entrega deve ser indicada, sem fabricar confirmação física.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-07; HOST + HIL):** Stop durante execução preserva ordem e dados finais, retém valor aceitável e permite nova execução. Distinguir parada da simulação, estado DAQC e falha de entrega/registro; limites pendentes no ICD.

**Design:** ARCH-STATE / IF-LOG. **Execução:** TASK-002, TASK-003, TASK-009. **Pendência:** DEC-005; Q-04/Q-07/Q-09.

### REQ-F-08 — Indicação do Estado da Simulação

A interface deve indicar pelo menos Idle, Running, Error, Stopped e Finished. Não há pausa manual requerida; proteção automática por excesso de inválidos precisa indicar sua condição, com pausa recuperável versus término em Error ainda a esclarecer.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-08; HOST + GUI):** Exercitar estados mínimos, fechamento de workers e nova execução; não declarar Finished antes de agregar resultado. Teste da proteção segue Q-03 sem criar Paused oficial por inferência.

**Design:** ARCH-STATE. **Execução:** TASK-003, TASK-009. **Pendência:** DEC-005/009; Q-03/Q-07.

### REQ-F-09 — Apresentação de Falhas

Falhas detectadas no código, FMU ou comunicação devem ser apresentadas na GUI com descrição específica e estado coerente. Reter a referência do último valor válido nas saídas, permitir recuperação para nova execução e não depender de prints de terminal para comunicar erro.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-09; HOST + HIL):** Injetar falha de FMU/código/link/log; GUI recebe causa, valor inválido não é enviado e repetição de run é possível após revalidação. Debug desligado não suprime diagnóstico GUI.

**Design:** ARCH-STATE / IF-LOG. **Execução:** TASK-002, TASK-003, TASK-009. **Pendência:** DEC-005; Q-03/Q-04/Q-07/Q-09.

### REQ-F-10 — Habilitação do Logging

A interface deve permitir habilitar/desabilitar logging binário dos valores finais das saídas selecionadas, uma amostra por passo. Métricas de desempenho não fazem parte do stream por passo. CSV é uma conversão posterior ao encerramento.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-10; HOST + GUI):** Desabilitado não cria stream de saída; habilitado grava binário sem CSV concorrente nem subpassos internos; estatísticas finais continuam acessíveis independentemente do stream.

**Design:** ARCH-GUI / IF-LOG. **Execução:** TASK-009. **Pendência:** DEC-002/008; Q-07.

### REQ-F-11 — Registro Temporal das Saídas

Com logging habilitado, registrar o valor final de cada saída selecionada correspondente à entrega de cada passo, com associação temporal identificável. Não registrar cada subpasso interno da FMU. Definição de valor bruto versus aceito e representação de tempo/ordem ficam explícitas no contrato de arquivo.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-11; HOST + HIL):** Confrontar passos concluídos e registros finais por saída; detectar perda e amostra final ausente. Q-07 fecha valor/tempo/metadados sem preencher lacunas com dados fictícios.

**Design:** ARCH-LOG / IF-LOG. **Execução:** TASK-002. **Pendência:** DEC-008; Q-07.

### REQ-F-12 — Coleta de métricas para apresentação final

A plataforma deve coletar as métricas temporais e de falha necessárias à apresentação final de desempenho, separadamente do stream binário de saídas. O logging por passo deve conter apenas saídas conforme DEC-008; persistência adicional de métricas não é requerida por esta revisão.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-12; HOST + alvo):** Cenário controlado produz agregados corretos e os apresenta no fim; habilitar/desabilitar logging não muda cálculos. Não exigir métricas por passo dentro do binário de saídas.

**Design:** ARCH-TIME / IF-LOG. **Execução:** TASK-002, TASK-004. **Pendência:** DEC-004/008; Q-06.

### REQ-F-13 — Apresentação dos Resultados de Desempenho

Ao terminar normalmente, por Stop ou erro, apresentar resultados disponíveis: quantidade de deadlines perdidos, pior atraso de entrega observado, quantidade de timeouts USB, médias e máximos observados de ciclo, processamento FMU e leitura/escrita USB. Ausência de medição não deve ser exibida como valor medido zero.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-13; HOST + HIL):** Injetar atrasos conhecidos e conferir contagens/médias/máximos/pior perda. Simulação continua após overrun. Campos USB desabilitados são identificados como não medidos.

**Design:** ARCH-TIME. **Execução:** TASK-004. **Pendência:** DEC-004; Q-06.

### REQ-F-14 — Plotagem das Variáveis de Saída

Permitir abrir uma janela individual por saída antes de Play ou durante execução, por controle distinto da configuração do gráfico. Fechar destrói janela e histórico próprio; reabrir coleta apenas a partir da reabertura, sem restaurar histórico anterior e sem interromper logging/simulação.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-14; GUI):** Abrir antes de Play: gráfico vazio até amostras; fechar/reabrir em Running não reapresenta dados antigos; várias janelas independentes. Retenção de configurações não recria histórico destruído.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** DEC-009.

### REQ-F-15 — Configuração dos Gráficos

Permitir mínimo, máximo e espaçamento das marcações do eixo Y por gráfico. Resolução significa espaçamento de ticks: −2…2 com resolução 1 gera −2, −1, 0, 1, 2. Uma janela temporal configurável é comum a todos os gráficos; ao exceder o limite superior, ambos os limites avançam mantendo a extensão.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-15; GUI):** Conferir exemplo Y, rejeitar espaçamento não positivo/não finito e limites inválidos; gráficos abertos compartilham a mesma janela temporal deslizante. Quantidade de amostras não é o espaçamento Y.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** DEC-009.

### REQ-F-16 — Controle da Plotagem em Tempo Real

A janela de simulação deve disponibilizar um controle que permita habilitar ou desabilitar a atualização dos gráficos em tempo real sem interromper a execução da simulação.

**Critério de aceitação proposto (V-F-16; GUI + alvo):** Desabilitar/reativar atualização durante Running; passos continuam e estado se mantém; ao reativar, exibir dados conforme política de janela sem backlog ilimitado.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** DEC-009.

### REQ-F-17 — Entradas Virtuais

A janela de simulação deve disponibilizar controles de entradas virtuais adequados ao tipo da variável, incluindo controles para valores reais, inteiros e booleanos, permitindo seu mapeamento às entradas da FMU.

**Critério de aceitação proposto (V-F-17; HOST + GUI):** Controles Real/Integer/Boolean atualizam a entrada mapeada na fronteira do ciclo, com tipo/faixa validados e sem corrida com atualização DAQC.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-006, TASK-009. **Pendência:** DEC-003/011.

### REQ-F-18 — Comunicação de Configuração com a DAQC

A plataforma deve suportar CONFIG no protocolo próprio, com SYNC de dois bytes 0x7259, MID 0x01 e COMMAND de um byte. No retorno ESP32→host existe STATUS para informar o estado efetivo, cuja codificação/tamanho ainda precisam da confirmação Q-02.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-18; HOST + bancada):** Vetores CONFIG com SYNC/MID/COMMAND definidos; confirmar bytes de SYNC, STATUS e fragmentação antes de teste de interoperabilidade. Não aceitar MID como codificação de três estados sem esclarecimento.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-012; Q-01/Q-02/Q-09.

### REQ-F-19 — Comunicação de Streaming com a DAQC

A plataforma deve suportar DATA com SYNC=0x7259, MID=0x02 e até 256 bytes de payload organizado por I/O do perfil. Não é obrigatório usar todos os bytes. STATUS só pode ocorrer no sentido ESP32→host; sua presença no DATA e o mecanismo de comprimento ainda precisam de Q-02.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-19; HOST + HIL):** Testar perfis/layout, frames parciais/agregados e limite do payload. Payload contendo os bytes de SYNC não pode ser truncado por busca ingênua; tamanho/STATUS definidos antes do parser definitivo.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-012; Q-01/Q-02/Q-05.

### REQ-F-20 — Controle de Estado da DAQC

A plataforma deve enviar comandos DISABLE=0x01, ENABLE=0x02 e STREAMING=0x03 e interpretar o estado retornado pela DAQC. Em STREAMING, CONFIG continua sendo processado; DISABLE deve interromper streaming e permitir liberar a comunicação, sem impedir futuros comandos.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-20; HOST + HIL):** Saturar streaming e enviar DISABLE; verificar transição e comunicação posterior sem reset. Não confundir eco com estado assumido; fechar normalmente e matar processo são cenários distintos.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007, TASK-008. **Pendência:** DEC-012; Q-02/Q-09.

### REQ-F-21 — Validação da Configuração antes da Execução

Antes de executar, validar FMU, passo/duração, perfil DAQC, mapeamento I/O e configuração binária quando utilizada. Incompatibilidades devem informar em qual variável, tipo, recurso ou parâmetro ocorre a divergência e impedir aplicação parcial/Play inválido.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-21; HOST):** Carregar configurações com variável removida, tipo alterado, perfil divergente, função GPIO em conflito e timing inválido; obter diagnóstico específico antes de atuar.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-003, TASK-006. **Pendência:** DEC-011; Q-04/Q-05/Q-08/Q-09.

### REQ-F-22 — Finalização Consistente do Logging

Ao finalizar uma simulação por término normal, Stop ou erro recuperável, a plataforma deve fechar de forma consistente os arquivos de logging e preservar os dados registrados até o último ciclo concluído.

**Critério de aceitação proposto (V-F-22; HOST):** Em fim/Stop/erro recuperável, preservar até última amostra concluída quando sink disponível; testar interleaving final e erros de flush/close; impossibilidade de gravação deve ser erro explícito, sem alegar preservação impossível.

**Design:** ARCH-LOG / IF-LOG. **Execução:** TASK-002. **Pendência:** DEC-008.


### REQ-F-23 — Inicialização e retenção do último valor válido

A saída deve usar o valor inicial/start do modelo na inicialização pretendida e reter o último valor aceitável em Stop, fim, erro e valor inválido. NaN/Inf e outros valores inválidos não devem ser enviados. Boot físico sem modelo/valor e start ausente/calculado são casos pendentes explícitos, não preenchidos com zero.

**Origem 0.2:** comportamento novo solicitado nas respostas DEC; ID acrescentado sem renumerar requisitos originais.

**Critério de aceitação proposto (V-F-23; HOST + bancada/HIL):** Testar start válido, outputs calculados sem start, NaN/Inf/faixa e repetição de run. Verificar retenção por canal sem declarar entrega física se o link falhar.

**Design:** ARCH-STATE / IF-SAMPLE. **Execução:** TASK-003, TASK-006, TASK-008. **Pendência:** DEC-005; Q-04/Q-05/Q-09.

### REQ-F-24 — Proteção configurável contra valores inválidos consecutivos

GUI deve disponibilizar checkbox que habilita proteção ao ocorrerem mais de 100 valores inválidos consecutivos, com mensagem específica. O usuário pediu pausa automática e ausência de pausa manual; a ação pausa recuperável versus término em Error e o escopo do contador ficam pendentes em Q-03.

**Origem 0.2:** comportamento novo solicitado nas respostas DEC; ID acrescentado sem renumerar requisitos originais.

**Critério de aceitação proposto (V-F-24; HOST + GUI):** Casos 100 e 101 inválidos, checkbox ligado/desligado e retorno a válido; verificar mensagem e retenção. Contagem e transição finais só após Q-03, sem inventar Paused oficial.

**Design:** ARCH-STATE / IF-SAMPLE. **Execução:** TASK-003, TASK-009. **Pendência:** DEC-005/009; Q-03.

### REQ-F-25 — Modo debug e diagnóstico desacoplado

Prints de código próprio no terminal só são permitidos quando debug estiver habilitado; núcleo pode ser operado por terminal nesse modo. Erros devem ser entregues à GUI independentemente de prints, e o caminho crítico não deve realizar I/O textual nem quando debug estiver ligado.

**Origem 0.2:** comportamento novo solicitado nas respostas DEC; ID acrescentado sem renumerar requisitos originais.

**Critério de aceitação proposto (V-F-25; HOST + GUI):** Com debug desligado, capturar saída própria e conferir ausência de prints; GUI recebe erro. Com debug ligado, mensagens são consumidas fora do ciclo. Auditar callbacks FMI e distinguir prints internos de terceiros.

**Design:** ARCH-STATE / ARCH-LOG. **Execução:** TASK-002, TASK-003, TASK-009. **Pendência:** Encaminhamento de mensagens internas de terceiros a verificar; requisito próprio confirmado.

### REQ-F-26 — Conversão do registro binário em CSV após execução

A plataforma deve disponibilizar conversão do binário de saídas para CSV somente após o término da execução, utilizando a FMU como referência dos tipos. Validar identidade/ordem de saídas do registro; mecanismo exato de metadados e tratamento de encerramento por erro ficam no contrato.

**Origem 0.2:** comportamento novo solicitado nas respostas DEC; ID acrescentado sem renumerar requisitos originais.

**Critério de aceitação proposto (V-F-26; HOST + GUI):** Converter depois de Finished/Stopped conforme contrato, bloquear conversão durante Running e rejeitar FMU incompatível. Comparar valores tipados do binário/CSV sem depender da ordem presumida do XML.

**Design:** ARCH-LOG / IF-LOG. **Execução:** TASK-002, TASK-009. **Pendência:** DEC-002/008; Q-07/Q-08.

## Requisitos não funcionais

### REQ-NF-01 — Plataforma de Execução

Aplicação principal deve executar em Linux Ubuntu 22.04; etapa inicial em máquina Linux com ROS 2 Humble e alvo posterior Raspberry Pi 4 de 2 GB. Arquitetura do SO/binários e versões do alvo precisam ser identificadas antes de validar o Pi.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-01; HOST + alvo):** Build e execução identificados no Linux inicial; ensaio posterior no Pi 4/2 GB com FMU da arquitetura correta. Teste HOST não aprova automaticamente o alvo.

**Design:** ARCH-CORE. **Execução:** TASK-001. **Pendência:** DEC-006.

### REQ-NF-02 — ROS 2

A integração ROS 2 do projeto deve utilizar a distribuição ROS 2 Humble.

**Critério de aceitação proposto (V-NF-02; HOST + HIL):** Componentes integram grafo ROS 2 Humble e trocam mensagens contratadas com versão de pacote registrada; sem substituir Humble por distro não aprovada.

**Design:** ARCH-IO. **Execução:** TASK-007. **Pendência:** DEC-001/006/012.

### REQ-NF-03 — Execução em Tempo Real da Simulação

O loop principal de simulação deve ser executado em thread dedicada, utilizando pthreads e política de escalonamento SCHED_FIFO com prioridade superior às threads não críticas.

**Critério de aceitação proposto (V-NF-03; HOST + alvo):** Durante run conforme requisito, consultar política/prioridade efetivas da thread dedicada e comparar às não críticas; falha de SCHED_FIFO não aprova execução conforme NF-03.

**Design:** ARCH-TIME. **Execução:** TASK-004. **Pendência:** DEC-007.

### REQ-NF-04 — Thread de Leitura USB

A leitura USB deve ser executada em thread dedicada de alta prioridade, de forma contínua enquanto a DAQC estiver habilitada e houver variáveis da FMU mapeadas para entradas provenientes da DAQC.

**Critério de aceitação proposto (V-NF-04; HOST + alvo):** Com DAQC habilitada e entradas mapeadas, leitura ocorre em thread dedicada prioritária conforme redação vigente; desabilitação encerra leitura sem bloqueio ilimitado. Revisar teste se transporte mudar.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-001/004/007.

### REQ-NF-05 — Transferência USB

A comunicação USB 2.0 entre a main board e a DAQC deve utilizar transferências do tipo Bulk.

**Critério de aceitação proposto (V-NF-05; Bancada):** Captura/descritores e caminho efetivo demonstram Bulk se mantido; conector USB e porta TTY isolados não provam conformidade da implementação.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-001.

### REQ-NF-06 — Biblioteca USB

A comunicação USB da main board deve utilizar libusb, mantendo a exigência atual de implementação da camada em C. Leitura/orquestração em Python foi levantada como possibilidade; seu papel e eventual revisão desta exigência dependem de Q-01.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-06; HOST + bancada):** Identificar código/caminho libusb e dono do dispositivo. Se Python for adotado, registrar como interage com camada C e snapshots; não marcar um leitor serial comum como conformidade de libusb.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-001; Q-01.

### REQ-NF-07 — Escrita USB no Ciclo de Simulação

A escrita das saídas destinadas à DAQC deve ocorrer ao final do ciclo de simulação correspondente, preservando a ordem temporal entre aquisição de entradas, processamento da FMU e publicação das saídas.

**Critério de aceitação proposto (V-NF-07; HOST + HIL):** Instrumentar ciclo n e demonstrar escrita do conjunto de saídas correspondente depois do processamento e antes do ciclo seguinte, dentro do limite contratado.

**Design:** ARCH-TIME / IF-DAQ. **Execução:** TASK-004, TASK-007. **Pendência:** DEC-001/004/007.

### REQ-NF-08 — Proteção de Dados Compartilhados

O compartilhamento de dados entre as threads de leitura USB e de simulação deve utilizar double-buffering, com mutex restrito à troca/seleção do buffer e às regiões críticas necessárias.

**Critério de aceitação proposto (V-NF-08; HOST + alvo):** Sob escritor/leitor concorrentes, snapshot nunca mistura gerações; mutex limitado à troca/cópia necessária e sem I/O sob lock. Medir bloqueio máximo aprovado.

**Design:** ARCH-IO / IF-SAMPLE. **Execução:** TASK-004, TASK-007. **Pendência:** DEC-001/004.

### REQ-NF-09 — Atualização da Interface Gráfica

Atualização visual dos gráficos deve ocorrer no máximo a 10 Hz, desacoplada do ciclo HiL, sem obrigar perder amostras de logging ou reduzir a frequência do núcleo.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-09; GUI + alvo):** Contar atualizações por janela durante run e verificar teto de 10 Hz; atraso de GUI pode reduzir renderização sem bloquear simulação; histórico respeita janela/vida do gráfico.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** DEC-009.

### REQ-NF-10 — Prioridade da Interface Gráfica

A interface gráfica deve ser executada em thread distinta das threads críticas de simulação e comunicação, utilizando prioridade não real-time ou inferior às threads críticas.

**Critério de aceitação proposto (V-NF-10; HOST + alvo):** Identificar thread GUI distinta e política não RT/inferior à crítica; bloquear atualização em ensaio não desloca ciclo além do critério temporal aprovado.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** DEC-004.

### REQ-NF-11 — Linguagem do Núcleo de Simulação

As funções responsáveis pelo núcleo de simulação, gerenciamento da FMU e execução da thread de simulação devem ser implementadas em linguagem C.

**Critério de aceitação proposto (V-NF-11; HOST):** Inspeção/build comprovam fontes C do núcleo/wrapper/thread; API GUI/ROS não transfere a execução FMU para código de interface.

**Design:** ARCH-CORE. **Execução:** TASK-001. **Pendência:** Nenhuma.

### REQ-NF-12 — Interface Gráfica

GUI deve utilizar Qt 6/C++, desacoplada do núcleo C, que pode operar pelo terminal em modo debug. A aparência deve lembrar Linux Mint; APIs Qt são preservadas e estilo próprio segue constituição.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-12; HOST + GUI):** Executar cenário pelo núcleo sem GUI e pela GUI; conferir controles/cor e baixa prioridade, sem dependência de widgets no núcleo. Cores específicas são design proposto, não tokens oficiais Mint exigidos.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** Nenhuma escolha de toolkit/estilo pendente; detalhes de GUI em Q-03/Q-07.

### REQ-NF-13 — Biblioteca FMI

A manipulação de FMUs 2.0 deve utilizar a biblioteca FMILibrary (FMILib) ou camada de abstração construída sobre ela.

**Critério de aceitação proposto (V-NF-13; HOST):** Identificar versão FMILibrary vinculada; wrapper executa fixture por essa biblioteca/camada; não usar binário legado como evidência da dependência atual.

**Design:** ARCH-CORE. **Execução:** TASK-001. **Pendência:** DEC-006/011.

### REQ-NF-14 — Biblioteca de Abstração para FMU

O projeto deve possuir uma biblioteca complementar à FMILibrary para encapsular operações recorrentes, incluindo importação e destruição de FMUs, tratamento de logs/erros e identificação dos tipos das variáveis de entrada e saída.

**Critério de aceitação proposto (V-NF-14; HOST):** Camada encapsula importar/destruir, erros e tipos de inputs/outputs; falha em cada fase libera recursos e permite próxima execução válida.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-003, TASK-006. **Pendência:** DEC-011.

### REQ-NF-15 — Arquitetura Modular

O software deve ser estruturado em módulos separados, no mínimo, para interface gráfica, núcleo de simulação/FMU e comunicações USB/ROS 2, reduzindo acoplamento entre subsistemas.

**Critério de aceitação proposto (V-NF-15; HOST):** Inspecionar dependências e executar núcleo sem GUI/ROS; componentes de comunicação e GUI usam interfaces sem chamar a instância FMU diretamente.

**Design:** ARCH-CORE / ARCH-IO / ARCH-GUI. **Execução:** TASK-003, TASK-007, TASK-009. **Pendência:** DEC-001.

### REQ-NF-16 — Extensibilidade de Perfis de DAQC

Características DAQC devem ser modulares por perfil; o primeiro chama-se ESP32 e disponibiliza recursos do módulo informado com ADC/DAC internos e restrições de pinagem/multiplexação. Novos perfis não exigem alteração extensiva do núcleo.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-16; HOST + bancada):** Catálogo identifica capacidades/exclusões; não permitir dois usos incompatíveis de um pino. Dois perfis sintéticos testam extensibilidade e perfil real é verificado separadamente.

**Design:** ARCH-IO / IF-CORE. **Execução:** TASK-006, TASK-008. **Pendência:** DEC-003; Q-05.

### REQ-NF-17 — Aplicação da DAQC em micro-ROS

A aplicação executada no ESP32 deve utilizar micro-ROS, com mensagens e tópicos compatíveis com a interface ROS 2 da main board.

**Critério de aceitação proposto (V-NF-17; Bancada + HIL):** Firmware do ESP32 informado comunica por micro-ROS com host Humble usando mensagens compatíveis; demonstrar o agente/transporte definido em Q-01, não apenas publicação por bridge de frames próprios.

**Design:** ARCH-FW / IF-DAQ. **Execução:** TASK-008. **Pendência:** DEC-001/006/012.

### REQ-NF-18 — Organização dos Ambientes ROS

Os componentes ROS 2/micro-ROS da main board e da DAQC devem ser mantidos em diretórios próprios e isolados do núcleo principal da aplicação, quando aplicável.

**Critério de aceitação proposto (V-NF-18; HOST):** Pacotes host/firmware/mensagens ficam em diretórios próprios e não impõem headers ROS ao núcleo independente; build separado exercitado.

**Design:** ARCH-IO. **Execução:** TASK-001, TASK-007. **Pendência:** DEC-001.

### REQ-NF-19 — Compatibilidade das Mensagens de I/O

As mensagens de comunicação com a DAQC devem representar a quantidade e os tipos de I/O suportados pelo perfil do ESP32/DAQC, incluindo entradas e saídas digitais e analógicas.

**Critério de aceitação proposto (V-NF-19; HOST + HIL):** Mensagem representa cada canal habilitado e tipo do perfil sem omissão/troca de direção; receptor rejeita perfil ou versão incompatível.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-006, TASK-007, TASK-008. **Pendência:** DEC-003/012.

### REQ-NF-20 — Codificação Numérica

Valores reais de 32 bits transmitidos pelo protocolo devem utilizar representação IEEE 754 float32.

**Critério de aceitação proposto (V-NF-20; HOST + bancada):** Vetores conhecidos demonstram float32 IEEE 754 e byte order contratado; zeros, extremos e valores não finitos seguem política definida, não memcpy dependente do host.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-012.

### REQ-NF-21 — Estrutura do Protocolo por Perfil

A organização das variáveis no PAYLOAD USB deve ser definida pelo perfil da DAQC, de forma determinística e documentada, permitindo que main board e DAQC interpretem o mesmo layout.

**Critério de aceitação proposto (V-NF-21; HOST + HIL):** Dado perfil versionado, host e DAQC produzem/interpretam layout idêntico, offsets/tamanhos explícitos; versão errada é rejeitada.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-003/012.

### REQ-NF-22 — Campos de Sincronização do Protocolo

Protocolo deve usar SYNC fixo de dois bytes 0x7259 e MID de um byte, CONFIG=0x01 e DATA/PAYLOAD=0x02. A ordem dos bytes de SYNC deve ser definida explicitamente no ICD, não inferida da arquitetura do processador.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-22; HOST + bancada):** Vetores usam novo SYNC e MIDs exatos; confirmar ordem no fio em Q-02. BB77 permanece somente na referência histórica, não é o SYNC vigente.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-012; Q-02.

### REQ-NF-23 — Validação de Frames

O receptor deve validar os campos SYNC e MID antes de interpretar COMMAND, STATUS ou PAYLOAD.

**Critério de aceitação proposto (V-NF-23; HOST + bancada):** SYNC/MID inválidos nunca levam à interpretação/aplicação de comando/payload; testar fragmentos, ruído e recuperação de fronteira.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-001/012.

### REQ-NF-24 — Comandos do Protocolo

COMMAND possui um byte: 0x01 DISABLE, 0x02 ENABLE (IDLE), 0x03 STREAMING. Comandos de configuração devem continuar sendo interpretados durante streaming, inclusive DISABLE.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-24; HOST + HIL):** Exercitar três comandos e comandos inválidos; DISABLE em carga interrompe streaming e mantém recepção disponível. STATUS não é definido por inferência de MID.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007, TASK-008. **Pendência:** DEC-012; Q-02/Q-09.

### REQ-NF-25 — Tamanho máximo do payload de dados

PAYLOAD de dados deve ter no máximo 256 bytes, usando somente a quantidade definida pelo perfil. Esse limite não inclui SYNC/MID nem STATUS ou outros campos que sejam posteriormente aprovados; não limita o frame inteiro a 256 bytes.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-25; HOST + bancada):** Testar payload máximo e excesso; um payload 256 com SYNC/MID tem 259 bytes antes de STATUS. Parser dimensiona frame pela definição efetiva, sem overflow nem depender de um read por frame.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** DEC-012; Q-02.

### REQ-NF-26 — Convenções de Código C

No código C próprio, as funções devem utilizar PascalCase; variáveis locais/globais e campos devem utilizar snake_case; constantes e macros devem utilizar UPPER_CASE; estruturas e tipos customizados devem utilizar snake_case terminado em `_t`; arquivos devem utilizar snake_case. Nomes exigidos por ABI e bibliotecas externas permanecem inalterados.

**Alteração confirmada:** substitui a convenção da referência A por instrução explícita do usuário; ver ADR-001.

**Critério de aceitação proposto (V-NF-26; Revisão + HOST):** Funções próprias C PascalCase, variáveis/campos snake_case, tipos próprios snake_case_t, macros/constantes UPPER_CASE, arquivos snake_case; ABI/terceiros preservados e build/testes sem mudança funcional.

**Design:** ADR-001 / constitution. **Execução:** TASK-005. **Pendência:** Nenhuma.

### REQ-NF-27 — Convenções de Código Python

Nos componentes Python próprios, variáveis, funções e métodos devem utilizar snake_case; classes devem utilizar PascalCase; constantes devem utilizar UPPER_CASE; módulos e pacotes devem ter nomes curtos em minúsculas, usando snake_case quando melhorar a leitura.

**Complementação confirmada:** tabela de nomenclatura fornecida pelo usuário; regra de quebra de linha segue a constituição, sem imposição cosmética de largura.

**Critério de aceitação proposto (V-NF-27; Revisão + HOST):** Quando houver Python de produto, funções/métodos/variáveis snake_case, classes PascalCase, constantes UPPER_CASE e módulos conforme regra; sem inventar componente Python para satisfazer item.

**Design:** ADR-001 / constitution. **Execução:** TASK-005. **Pendência:** Nenhuma.

### REQ-NF-28 — Detecção de Deadline

Núcleo deve medir ciclo e perdas de deadline, continuar a simulação quando ocorrer perda, contar ocorrências e informar pior atraso observado ao final. Duração do trabalho e atraso de entrega/despertar precisam ser distinguíveis; agenda após atraso deve ser explicitada.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-28; HOST + alvo):** Clock controlado produz perda conhecida e run continua; conferir contador e pior atraso e não confundir máximo computacional com lateness. Q-06 fecha agenda/cálculo operacional.

**Design:** ARCH-TIME. **Execução:** TASK-004. **Pendência:** DEC-004/007; Q-06.

### REQ-NF-29 — Detecção de Timeout USB

Comunicação deve ter timeout configurado positivo de no máximo 5 ms, detectar/contabilizar falhas de leitura/escrita sem bloquear indefinidamente a thread de simulação. Escopo por transferência ou por troca completa deve ser fechado em Q-06; valor infinito não atende.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-29; HOST + HIL):** Injetar atraso e verificar configuração e tempo observado; testar limites positivos até 5 ms, rejeitar infinito/timeout sem limite. Registrar que configuração de API não prova limite sob scheduler.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-004, TASK-007. **Pendência:** DEC-004; Q-06.

### REQ-NF-30 — Isolamento de Falhas da Interface

Falhas ou atrasos na atualização da interface gráfica e na plotagem não devem bloquear nem alterar a cadência do loop principal de simulação.

**Critério de aceitação proposto (V-NF-30; HOST + alvo):** Atrasar/parar consumidor gráfico em ensaio com carga; ciclo não executa I/O de GUI nem espera por ela; medir cadência contra tolerância aprovada.

**Design:** ARCH-LOG / ARCH-GUI. **Execução:** TASK-002, TASK-004, TASK-009. **Pendência:** DEC-004/007.

### REQ-NF-31 — Consistência Temporal do Ciclo

Cada ciclo de simulação deve consumir um conjunto consistente de entradas, executar uma única etapa da FMU e publicar um conjunto correspondente de saídas antes do início do ciclo seguinte.

**Critério de aceitação proposto (V-NF-31; HOST + HIL):** Cada ciclo tem snapshot consistente, um doStep e publicação do conjunto correspondente antes do seguinte; injetar atualização concorrente, stale e escrita tardia e observar política contratada.

**Design:** ARCH-TIME / IF-SAMPLE. **Execução:** TASK-004, TASK-006, TASK-007. **Pendência:** DEC-004/007/012.

### REQ-NF-32 — Meta de desempenho de pelo menos 100 Hz

A plataforma deve ter como meta executar simulações a pelo menos 100 Hz (passo de referência de 10 ms), configuráveis, sem depender de uma planta fixa. Cumprimento deve ser medido para o modelo/host/perfil ensaiado; importar qualquer FMU 2.0 CS não garante que todo modelo caiba nesse orçamento.

**Origem 0.2:** comportamento novo solicitado nas respostas DEC; ID acrescentado sem renumerar requisitos originais.

**Critério de aceitação proposto (V-NF-32; HOST + HIL):** Ensaiar fixtures documentadas a 100 Hz, medir ciclo completo, perda/pior atraso e comunicação. Informar duração/carga e critérios de desempenho pendentes, sem aprovar meta só por configurar h=0,01.

**Design:** ARCH-TIME. **Execução:** TASK-004, TASK-010. **Pendência:** DEC-004; Q-06/Q-08.

## Critérios transversais e lacunas

Implementação não iniciada: resolver Q-01…09 antes de avançar para código conforme instrução do usuário. Confirmações de formato/produto já estão aplicadas; arquitetura XRCE, STATUS/comprimento, boot sem start, pausa automática, recursos simultâneos, timing pós-atraso e falha de log não são fechados por suposição.

Suporte geral FMI exige distinguir metadados/tipos, capacidades e binário de execução da meta de desempenho. Timestamps/ordem do binário não se inferem apenas da FMU. A retenção do último válido não fabrica um valor antes do primeiro modelo nem prova entrega após desconexão. Valores didáticos não são usados como limites do produto.

## Histórico de trabalho

| Revisão | Alteração | Natureza |
|---|---|---|
| 0.1 | 53 requisitos importados; NF-26/27 revisados; critérios propostos | Instrução inicial e ADR-001 |
| 0.2 | Respostas DEC aplicadas, cinco IDs derivados, novos SYNC/limite payload/teto gráfico/log | Instrução explícita do usuário e ADR-002 |
| 0.2 | Lacunas Q mantidas, sem implementação nem aprovação de testes | Consolidação documental em andamento |
