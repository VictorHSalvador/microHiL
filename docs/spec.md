# Especificação de trabalho do MICROHIL

Referência de origem: MICROHIL-REQ-001-A, versão 01 de 24.08.2026. Revisão de trabalho: 0.8, baseline conjunta [SDD-MICROHIL 0.8.0](sdd-versions.md), 09.09.2026. **Estado: decisões de produto e ICD consolidados; TASK-001 e TASK-002 verificadas no HOST, validação de produto pendente.** Este é o documento editável para a retomada solicitada; não é uma revisão A retroativamente alterada nem declaração de validação.

## Origem e regra de leitura

Os 53 IDs de origem foram preservados. Respostas DEC-001…012 atualizam os textos afetados nesta revisão, conforme [ADR-002](adrs/ADR-002-product-decisions.md). Seis requisitos derivados foram acrescentados: F-23…27 e NF-32, totalizando 59. Respostas Q-01…09 refinam esta revisão. A [referência A](references/MICROHIL-REQ-001-A.md) continua intacta; estilo C/Python foi confirmado na [ADR-001](adrs/ADR-001-coding-style.md).

**Texto vigente:** exigência recebida ou alteração confirmada; detalhes físicos desconhecidos não foram inventados. **Critério de aceitação proposto:** refinamento para teste, não evidência executada. **Pendência:** parâmetro de implementação, medição ou dependência externa; não reabre decisões confirmadas. A implementação começa somente após a verificação cruzada final dos Markdown.

Confirmados: Qt Quick/QML desacoplado/terminal debug, micro-ROS sob coordenador C único em `/dev/ttyUSB*`, perfil ESP32/ADC-DAC/PWM, Pi 4/2 GB após Linux inicial, logging binário de saídas/CSV posterior, retenção de último válido, meta 100 Hz/timeout USB até 5 ms, continuar após overrun, gráficos até 10 Hz e DATA sem CRC/retransmissão. [decisions.md](decisions.md) separa decisões dos parâmetros a medir.

Comentários de todo código próprio devem estar em inglês, ser breves e úteis, conforme [constituição](constitution.md). Não quebrar parâmetros por estética; quebrar apenas linhas extremamente longas. Essas regras são instruções confirmadas do usuário, aplicáveis a todos os incrementos, sem criar funcionalidades ou mecanismos de hardware adicionais.

## Direção dos sinais — terminologia normativa

| Caminho | Significado |
|---|---|
| Mundo real → entradas físicas DAQC (AI/DI) → USB → host → inputs FMU | Aquisição. O dado transmitido pela DAQC é uma entrada física adquirida e alimenta uma entrada do modelo |
| Outputs FMU → host → USB → saídas físicas DAQC (AO/DO/PWM) → mundo real | Atuação. Saída física da DAQC sempre significa sinal entregue ao mundo real |

A retenção de F-23 e a contagem de F-24 pertencem à aquisição no host. A mensagem identifica canal DAQC e input FMU. Amostra inválida isolada não manda zero aos atuadores; quando a simulação termina (fim, Stop ou Error), as saídas físicas são zeradas e o envio de dados é encerrado. Novo Play reinicializa a FMU e aplica os outputs iniciais válidos, sem reutilizar outputs da execução anterior.

## Requisitos funcionais

### REQ-F-01 — Carregamento de FMU

A plataforma deve permitir importar modelos FMU 2.0 Co-Simulation sem depender de uma planta fixa, por botão e diálogo próprios na GUI, identificar nomes e tipos de entradas/saídas e verificar compatibilidade com o sistema antes de executar. Capacidades e binários incompatíveis devem ser diagnosticados especificamente, sem alegar suporte que não foi implementado.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-01; HOST):** Importar fixtures de diferentes modelos/tipos; identificar FMI/kind, nomes, tipos e arquitetura. Rejeitar arquivo incompatível com causa explícita. Q-08 autoriza diagnosticar capacidades não suportadas, sem restringir silenciosamente o produto à FMU de exemplo.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-003. **Pendência:** Matriz de capacidades por incremento; política Q-08 resolvida.

### REQ-F-02 — Configuração de Entradas e Saídas

A plataforma deve permitir associar inputs/outputs da FMU aos recursos do perfil DAQC selecionado, iniciando pelo perfil ESP32. O mapa deve respeitar tipo, direção, unidade/faixa e exclusividade de funções dos pinos; capacidades multiplexadas não são canais simultâneos independentes. ADC e PWM devem ser configuráveis pelo usuário com opções e limites explícitos do TARGET; rejeitar pares frequência/resolução e recursos incompatíveis antes de Play.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-02; HOST + bancada/HIL):** Mapear canais compatíveis e rejeitar funções conflitantes, direção/tipo/unidade incompatíveis; transmitir o descritor em DISABLE/ENABLE, confirmar o mesmo hash no firmware e bloquear STREAMING se houver divergência.

**Design:** ARCH-IO / IF-CORE. **Execução:** TASK-006. **Pendência:** caracterização elétrica e validação em bancada.

### REQ-F-03 — Seleção e Controle da DAQC

A plataforma deve permitir selecionar perfil DAQC compatível, inicialmente denominado ESP32, e habilitar/desabilitar sua participação na simulação. Os estados da DAQC são DISABLE, ENABLE (IDLE) e STREAMING, com confirmação de estado no protocolo a detalhar.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-03; HOST + HIL):** Selecionar ESP32, operar sem DAQC quando desabilitada e verificar transições/confirmação quando habilitada; DISABLE recebido em STREAMING interrompe fluxo sem bloquear comandos posteriores.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-006, TASK-008. **Pendência:** implementação e teste das transições.

### REQ-F-04 — Persistência do Perfil de Configuração

A interface deve salvar/carregar configuração em arquivo binário, incluindo I/O, mapa FMU–DAQC e parâmetros de execução. Carregamento usa botão distinto do de FMU e valida o arquivo contra a FMU carregada e a DAQC selecionada, informando quais variáveis, tipos, recursos ou parâmetros divergem antes de aplicar a configuração.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-04; HOST + GUI):** Round-trip completo; arquivos incompatíveis/corrompidos não aplicam configuração parcial e apresentam divergência por campo. Abrir configuração não substitui silenciosamente a FMU ou DAQC selecionada.

**Design:** ARCH-GUI / IF-LOG. **Execução:** TASK-009. **Pendência:** vetores do formato e testes de compatibilidade.

### REQ-F-05 — Configuração da Execução da Simulação

GUI e terminal debug devem permitir definir passo e duração da simulação, mantendo coerência e validando valores finitos positivos e compatibilidade com capacidades da FMU. O produto não exige uma planta ou passo fixos.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-05; HOST + GUI):** NaN/Inf/zero/negativo/overflow rejeitados; passo e duração configurados são usados. Verificar último passo parcial e capacidades da FMU sem fixar o modelo de exemplo como única planta.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-003, TASK-009. **Pendência:** matriz de capacidades FMI do incremento.

### REQ-F-06 — Início da Simulação

A plataforma deve iniciar a execução da simulação quando o usuário acionar o comando Play, desde que as configurações necessárias estejam válidas.

**Critério de aceitação proposto (V-F-06; HOST + GUI):** Play válido inicia uma única execução; Play inválido não executa passo nem habilita atuação; chamadas repetidas não criam runners concorrentes.

**Design:** ARCH-STATE / IF-CORE. **Execução:** TASK-003, TASK-009. **Pendência:** implementação e fixtures de lifecycle.

### REQ-F-07 — Parada da Simulação

Stop deve interromper a execução consistentemente, finalizar registros disponíveis, zerar as saídas físicas DAQC e encerrar envio de DATA. O zero de encerramento é independente da retenção dos inputs no host. Novo Play reinicializa a FMU e aplica seus outputs iniciais válidos antes de continuar a simulação. Confirmar atuação/encerramento pelo contrato, sem alegar entrega após falha do enlace.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-07; HOST + HIL):** Em Stop, fim e Error, verificar zero nos canais de saída e cessação de DATA; rejeitar atualização antiga posterior ao encerramento. Repetir Play com FMU reinicializada e verificar outputs iniciais da nova execução. Falha de entrega gera diagnóstico, não confirmação fictícia.

**Design:** ARCH-STATE / IF-LOG. **Execução:** TASK-002, TASK-003, TASK-009. **Pendência:** confirmação física do zero e tratamento da falha de enlace.

### REQ-F-08 — Indicação do Estado da Simulação

A interface deve indicar pelo menos Idle, Running, Error, Stopped e Finished. Não há pausa manual requerida; proteção automática por excesso de inválidos precisa indicar sua condição, com término em Error, identificação dos canais DAQC/inputs FMU afetados e possibilidade de novo Play (Q-03).

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-08; HOST + GUI):** Exercitar estados mínimos, fechamento de workers e nova execução; não declarar Finished antes de agregar resultado. Testar proteção em Error e novo Play; não há estado Paused requerido.

**Design:** ARCH-STATE. **Execução:** TASK-003, TASK-009. **Pendência:** implementação e testes de transição/restart.

### REQ-F-09 — Apresentação de Falhas

Falhas detectadas no código, FMU ou comunicação devem ser apresentadas na GUI com descrição e estado coerentes. Para dado de aquisição inválido, aplicar F-23/F-24 no host e indicar canal DAQC/input FMU. Recuperação permite nova execução; não depender de prints. Falha da FMU/atuação é outra classe de erro e não herda automaticamente o contador de aquisição.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-09; HOST + HIL):** Injetar falha de FMU/código/link/log; GUI recebe causa, dado adquirido inválido não é inserido na FMU e repetição de run é possível após revalidação. Debug desligado não suprime diagnóstico GUI.

**Design:** ARCH-STATE / IF-LOG. **Execução:** TASK-002, TASK-003, TASK-009. **Pendência:** captura de erros de terceiros e testes de falha.

### REQ-F-10 — Habilitação do Logging

A interface deve permitir habilitar/desabilitar logging binário dos valores finais das saídas selecionadas, uma amostra por passo. Métricas de desempenho não fazem parte do stream por passo. CSV é uma conversão posterior ao encerramento.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-10; HOST + GUI):** Desabilitado não cria stream de saída; habilitado grava binário sem CSV concorrente nem subpassos internos; estatísticas finais continuam acessíveis independentemente do stream.

**Design:** ARCH-GUI / IF-LOG. **Execução:** TASK-009. **Pendência:** implementação do sink e controles.

### REQ-F-11 — Registro Temporal das Saídas

Com logging habilitado, registrar o valor final de cada saída selecionada correspondente à entrega de cada passo, com associação temporal identificável. Não registrar cada subpasso interno da FMU. Registrar o valor bruto da FMU, sem confundir com os inputs DAQC filtrados no host; inválidos usam NaN ou zero conforme representação tipada a fechar em IF-LOG. Falha de logging gera aviso de registro incompleto e a simulação continua.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-11; HOST + HIL):** Confrontar passos concluídos e registros finais por saída; detectar perda e amostra final ausente. Testar outputs registrados sem substituição por inputs retidos e falha de sink sem parar a simulação; Q-07 fecha representação tipada e metadados.

**Design:** ARCH-LOG / IF-LOG. **Execução:** TASK-002. **Pendência:** integração com FMU real, GUI e confirmação do ciclo/atuação de produto; vetores HOST tipados e falhas de sink foram executados em 08.09.2026.

### REQ-F-12 — Coleta de métricas para apresentação final

A plataforma deve coletar as métricas temporais e de falha necessárias à apresentação final de desempenho, separadamente do stream binário de saídas. O logging por passo deve conter apenas saídas conforme DEC-008; persistência adicional de métricas não é requerida por esta revisão.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-12; HOST + alvo):** Cenário controlado produz agregados corretos e os apresenta no fim; habilitar/desabilitar logging não muda cálculos. Não exigir métricas por passo dentro do binário de saídas.

**Design:** ARCH-TIME / IF-LOG. **Execução:** TASK-002, TASK-004. **Pendência:** instrumentação e validação dos agregados.

### REQ-F-13 — Apresentação dos Resultados de Desempenho

Ao terminar normalmente, por Stop ou erro, apresentar resultados disponíveis: quantidade de deadlines perdidos, pior atraso de entrega observado, quantidade de timeouts USB, médias e máximos observados de ciclo, processamento FMU e leitura/escrita USB. Ausência de medição não deve ser exibida como valor medido zero.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-13; HOST + HIL):** Injetar atrasos conhecidos e conferir contagens/médias/máximos/pior perda. Simulação continua após overrun. Campos USB desabilitados são identificados como não medidos.

**Design:** ARCH-TIME. **Execução:** TASK-004. **Pendência:** limites de aceitação por ambiente/carga.

### REQ-F-14 — Plotagem das Variáveis de Saída

Permitir abrir uma janela individual por saída antes de Play ou durante execução, por controle distinto da configuração do gráfico. Fechar destrói janela e histórico próprio; reabrir coleta apenas a partir da reabertura, sem restaurar histórico anterior e sem interromper logging/simulação.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-14; GUI):** Abrir antes de Play: gráfico vazio até amostras; fechar/reabrir em Running não reapresenta dados antigos; várias janelas independentes. Retenção de configurações não recria histórico destruído.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** implementação e teste GUI.

### REQ-F-15 — Configuração dos Gráficos

Permitir mínimo, máximo e espaçamento das marcações do eixo Y por gráfico. Resolução significa espaçamento de ticks: −2…2 com resolução 1 gera −2, −1, 0, 1, 2. Uma janela temporal configurável é comum a todos os gráficos; ao exceder o limite superior, ambos os limites avançam mantendo a extensão.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-15; GUI):** Conferir exemplo Y, rejeitar espaçamento não positivo/não finito e limites inválidos; gráficos abertos compartilham a mesma janela temporal deslizante. Quantidade de amostras não é o espaçamento Y.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** implementação e teste GUI.

### REQ-F-16 — Controle da Plotagem em Tempo Real

A janela de simulação deve disponibilizar um controle que permita habilitar ou desabilitar a atualização dos gráficos em tempo real sem interromper a execução da simulação.

**Critério de aceitação proposto (V-F-16; GUI + alvo):** Desabilitar/reativar atualização durante Running; passos continuam e estado se mantém; ao reativar, exibir dados conforme política de janela sem backlog ilimitado.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** implementação e medição de carga GUI.

### REQ-F-17 — Entradas Virtuais

A janela de simulação deve disponibilizar controles de entradas virtuais adequados ao tipo da variável, incluindo controles para valores reais, inteiros e booleanos, permitindo seu mapeamento às entradas da FMU.

**Critério de aceitação proposto (V-F-17; HOST + GUI):** Controles Real/Integer/Boolean atualizam a entrada mapeada na fronteira do ciclo, com tipo/faixa validados e sem corrida com atualização DAQC.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-006, TASK-009. **Pendência:** implementação dos controles tipados e arbitragem.

### REQ-F-18 — Comunicação de Configuração com a DAQC

A plataforma deve suportar CONFIG no protocolo próprio, com SYNC de dois bytes 0x7259, MID 0x01 e COMMAND de um byte. No retorno ESP32→host existe STATUS para informar o estado efetivo, com códigos iguais aos COMMAND (01/02/03); STATUS uint8 somente no retorno CONFIG; ordem little-endian conforme reuso RaspDAQ em ADR-003.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-18; HOST + bancada):** Vetores CONFIG com SYNC/MID/COMMAND definidos; usar bytes SYNC 59 72, COMMAND/STATUS uint8 e quadros 4/5 bytes. Confirmar idempotência, estado efetivo e prioridade sobre DATA; STATUS usa códigos COMMAND, não MID.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** prazo agregado/tentativas de CONFIG serão medidos; base Q-02 consolidada.

### REQ-F-19 — Comunicação de Streaming com a DAQC

A plataforma deve suportar DATA com SYNC=0x7259, MID=0x02 e até 256 bytes de payload organizado por I/O do perfil. Não é obrigatório usar todos os bytes. STATUS aparece somente em CONFIG de retorno. DATA usa SEQ uint16 e comprimento N fixo por perfil/direção, little-endian, sem preenchimento e sem CRC. DATA é best effort: não há confirmação individual, retransmissão, replay ou recuperação. Repetidos/antigos/incompletos/atrasados são descartados; salto de sequência contabiliza perda e aceita o quadro novo.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-19; HOST + HIL):** Testar perfis/layout, frames parciais/agregados e limite do payload. Payload contendo SYNC não pode ser truncado por busca ingênua; quadro fixo 5+N e STATUS fora do DATA. Injetar perda, gap, repetido, antigo e timeout: nenhum bloqueia o próximo pacote nem gera retransmissão.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** firmware e medição da ponte; framing e layout ESP32 consolidados no ICD.

### REQ-F-20 — Controle de Estado da DAQC

A plataforma deve enviar comandos DISABLE=0x01, ENABLE=0x02 e STREAMING=0x03 e interpretar o estado retornado pela DAQC. Em STREAMING, CONFIG continua sendo processado; DISABLE deve interromper streaming e permitir liberar a comunicação, sem impedir futuros comandos.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-20; HOST + HIL):** Saturar streaming e enviar DISABLE; verificar transição e comunicação posterior sem reset. Não confundir eco com estado assumido; fechar normalmente e matar processo são cenários distintos.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007, TASK-008. **Pendência:** implementação e teste sob saturação.

### REQ-F-21 — Validação da Configuração antes da Execução

Antes de executar, validar FMU, passo/duração, perfil DAQC, mapeamento I/O e configuração binária quando utilizada. Incompatibilidades devem informar em qual variável, tipo, recurso ou parâmetro ocorre a divergência e impedir aplicação parcial/Play inválido.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-F-21; HOST + integração):** Carregar configurações com variável removida, tipo alterado, perfil/hash divergente, função GPIO em conflito e timing inválido; obter diagnóstico específico antes de atuar e impedir STREAMING quando firmware rejeitar o descritor.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-003, TASK-006. **Pendência:** fixtures de incompatibilidade e perfil físico.

### REQ-F-22 — Finalização Consistente do Logging

Ao finalizar uma simulação por término normal, Stop ou erro recuperável, a plataforma deve fechar de forma consistente os arquivos de logging e preservar os dados registrados até o último ciclo concluído.

**Critério de aceitação proposto (V-F-22; HOST):** Em fim/Stop/erro recuperável, preservar até última amostra concluída quando sink disponível; testar interleaving final e erros de flush/close; impossibilidade de gravação deve ser erro explícito, sem alegar preservação impossível.

**Design:** ARCH-LOG / IF-LOG. **Execução:** TASK-002. **Pendência:** encerramento integrado com lifecycle FMU e atuação física; drenagem e falhas de flush/close foram exercitadas no HOST em 08.09.2026.


### REQ-F-23 — Retenção no host das entradas provenientes da DAQC

O host deve validar cada canal adquirido pela DAQC antes de inseri-lo no input FMU mapeado. NaN/Inf ou outro dado inválido é ignorado e não substitui o último valor válido desse canal. Reutilizar esse último valor na entrada do modelo; valor constante válido continua válido. Não zerar saídas físicas da DAQC por essa ocorrência. Antes do primeiro valor válido adquirido, usar o valor inicial da entrada FMU correspondente, desde que válido. Não substituir automaticamente por zero se essa referência não existir ou for inválida; validar a inicialização antes de executar a etapa dependente. Dados de processo continuam restritos ao Play/STREAMING; ausência de start literal não impede o lifecycle FMI quando a inicialização for calculada.

**Atualização 0.4:** correção explícita do usuário sobre direção dos sinais, retenção no host e supervisão de leitura.

**Critério de aceitação proposto (V-F-23; HOST + integração):** Enviar válido A, NaN, Inf, dado fora do contrato e válido B por canal: FMU deve consumir A,A,A,A,B. Um canal inválido não altera o valor válido de outro. Não emitir comando de zeramento físico como efeito do filtro. Testar primeira amostra inválida usando referência inicial válida da entrada FMU; referência inicial ausente/inválida não pode ser apresentada como último dado DAQC válido.

**Design:** ARCH-CORE / ARCH-IO / IF-SAMPLE. **Execução:** TASK-003, TASK-006, TASK-007. **Pendência:** limites físicos do mapa dependem de caracterização; referência inicial está definida.

### REQ-F-24 — Parada por 100 passos consecutivos com aquisição inválida

A proteção deve contar passos da simulação consecutivos com dado inválido recebido da DAQC, separadamente por canal mapeado para input FMU. Com checkbox habilitado, no 100º passo inválido consecutivo de qualquer canal, encerrar em Error, identificar todos os canais que atingiram o limite e seus inputs FMU, permitir novo Play após revalidação. Não contar pacotes como passos, nem subpassos internos da FMU. Valor válido quebra a sequência, inclusive se constante; novo run reinicia contadores. O checkbox permanece: desligado, a proteção não encerra a simulação, mas o host continua ignorando inválidos e retendo a referência válida. Ao encerrar em Error, executar o zeramento físico e cessação de DATA de F-07.

**Atualização 0.4:** correção explícita do usuário sobre direção dos sinais, retenção no host e supervisão de leitura.

**Critério de aceitação proposto (V-F-24; HOST + integração):** Conferir 99 passos sem disparo e Error no 100º; múltiplos pacotes em um passo contam no máximo uma vez por canal. Testar canais independentes e vários atingindo limite no mesmo passo, válido que interrompe sequência, Error e novo Play. Sem nova amostra, timeout é separado e não incrementa inválidos; conferir seleção da última atualização na inserção FMI.

**Design:** ARCH-CORE / ARCH-STATE / IF-SAMPLE. **Execução:** TASK-003, TASK-006, TASK-007, TASK-009. **Pendência:** implementação e testes 99/100 por canal.

### REQ-F-25 — Modo debug e diagnóstico desacoplado

Prints de código próprio no terminal só são permitidos quando debug estiver habilitado; núcleo pode ser operado por terminal nesse modo. Erros devem ser entregues à GUI independentemente de prints, e o caminho crítico não deve realizar I/O textual nem quando debug estiver ligado.

**Origem 0.2:** comportamento novo solicitado nas respostas DEC; ID acrescentado sem renumerar requisitos originais.

**Critério de aceitação proposto (V-F-25; HOST + GUI):** Com debug desligado, capturar saída própria e conferir ausência de prints; GUI recebe erro. Com debug ligado, mensagens são consumidas fora do ciclo. Auditar callbacks FMI e distinguir prints internos de terceiros.

**Design:** ARCH-STATE / ARCH-LOG. **Execução:** TASK-002, TASK-003, TASK-009. **Pendência:** Encaminhamento de mensagens internas de terceiros a verificar; requisito próprio confirmado.

### REQ-F-26 — Conversão do registro binário em CSV após execução

A plataforma deve disponibilizar conversão do binário de saídas para CSV somente após o término da execução, utilizando a FMU como referência dos tipos. Validar identidade/ordem de saídas do registro; mecanismo exato de metadados e tratamento de encerramento por erro ficam no contrato.

**Origem 0.2:** comportamento novo solicitado nas respostas DEC; ID acrescentado sem renumerar requisitos originais.

**Critério de aceitação proposto (V-F-26; HOST + GUI):** Converter depois de Finished/Stopped conforme contrato, bloquear conversão durante Running e rejeitar FMU incompatível. Comparar valores tipados do binário/CSV usando a ordem explicitamente registrada e conferida contra os metadados XML.

**Design:** ARCH-LOG / IF-LOG. **Execução:** TASK-002, TASK-009. **Pendência:** ação e apresentação GUI; round-trip, incompatibilidade e cauda truncada foram exercitados no HOST em 08.09.2026.

### REQ-F-27 — DISABLE após 60 segundos de streaming sem leitura pelo host

Enquanto estiver em STREAMING, a DAQC deve entrar automaticamente em DISABLE ao completar 60 segundos contínuos sem o host ler os dados transmitidos. Não há etapa anterior de zeramento das saídas. O host envia READ_ACK cumulativo com o último SEQ DAQC→host efetivamente lido; esse ACK não confirma validade numérica e não solicita retransmissão. A condição é ausência de avanço do READ_ACK, não ausência de mudança no valor adquirido nem simples ausência de heartbeat. Interromper streaming, descartar backlog e manter recepção CONFIG disponível. O firmware deve usar os dois núcleos para separar tarefas críticas e supervisão, com comunicação limitada e sem espera bloqueante na aquisição/atuação; medir interferência antes de validar tempo real.

**Atualização 0.4:** correção explícita do usuário sobre direção dos sinais, retenção no host e supervisão de leitura.

**Critério de aceitação proposto (V-F-27; HOST + bancada/HIL):** Simular falta de leitura desde o primeiro DATA e depois de leitura efetiva: não desabilitar antes de 60 s, solicitar transição ao atingir 60 s e medir atraso da supervisão. READ_ACK novo, inclusive de frame com NaN, renova progresso; ACK repetido/antigo e heartbeat não renovam. Ensaio deve suspender o consumidor, verificar buffers limitados, CONFIG responsivo, ausência de retransmissão e novo Play sem dados anteriores.

**Design:** ARCH-FW / IF-READ-PROGRESS. **Execução:** TASK-006, TASK-007, TASK-008, TASK-010. **Pendência:** cadência do READ_ACK e tolerância de transição serão dimensionadas e medidas.

## Requisitos não funcionais

### REQ-NF-01 — Plataforma de Execução

Aplicação principal deve executar em Linux Ubuntu 22.04; etapa inicial em máquina Linux com ROS 2 Humble e alvo posterior Raspberry Pi 4 de 2 GB. Arquitetura do SO/binários e versões do alvo precisam ser identificadas antes de validar o Pi.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-01; HOST + alvo):** Build e execução identificados no Linux inicial; ensaio posterior no Pi 4/2 GB com FMU da arquitetura correta. Teste HOST não aprova automaticamente o alvo.

**Design:** ARCH-CORE. **Execução:** TASK-001. **Pendência:** arquitetura do SO/kernel e versões reproduzíveis do alvo.

### REQ-NF-02 — ROS 2

A integração ROS 2 do projeto deve utilizar a distribuição ROS 2 Humble.

**Critério de aceitação proposto (V-NF-02; HOST + HIL):** Componentes integram grafo ROS 2 Humble e trocam mensagens contratadas com versão de pacote registrada; sem substituir Humble por distro não aprovada.

**Design:** ARCH-IO. **Execução:** TASK-007. **Pendência:** versões compatíveis do ROS 2/micro-ROS e testes do grafo.

### REQ-NF-03 — Execução em Tempo Real da Simulação

O loop principal de simulação deve ser executado em thread dedicada, utilizando pthreads e política de escalonamento SCHED_FIFO com prioridade superior às threads não críticas.

**Critério de aceitação proposto (V-NF-03; HOST + alvo):** Durante run conforme requisito, consultar política/prioridade efetivas da thread dedicada e comparar às não críticas; falha de SCHED_FIFO não aprova execução conforme NF-03.

**Design:** ARCH-TIME. **Execução:** TASK-004. **Pendência:** prioridade efetiva e privilégios no ambiente.

### REQ-NF-04 — Thread de Leitura USB

A leitura USB deve ser executada em thread dedicada de alta prioridade, para dados de processo durante STREAMING iniciado por Play, quando houver inputs mapeados. Recepção de controle/status continua disponível fora de STREAMING.

**Critério de aceitação proposto (V-NF-04; HOST + alvo):** Com DAQC em STREAMING e entradas mapeadas, leitura ocorre em thread dedicada prioritária conforme redação vigente; desabilitação encerra leitura sem bloqueio ilimitado. Revisar teste se transporte mudar.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** prioridade efetiva, encerramento e medição de bloqueio.

### REQ-NF-05 — Transferência USB

A comunicação USB entre a main board e a DAQC deve usar a ponte CH340 pelo driver serial Linux, sob propriedade exclusiva do coordenador C.

**Critério de aceitação proposto (V-NF-05; HOST + bancada):** O coordenador abre exclusivamente o dispositivo CH340 identificado, configura o enlace e transmite/recebe os quadros ICD; dispositivo TTY isolado não prova interoperabilidade.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** descritores/endpoints reais e ensaio da ponte.

### REQ-NF-06 — Biblioteca USB

A comunicação host–CH340 deve usar a API serial POSIX em C, através de `/dev/ttyUSB*`, mantendo um único coordenador proprietário do enlace. O Agent micro-ROS recebe XRCE somente pelo transporte customizado conectado ao MID 04.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-06; HOST + bancada):** Identificar o coordenador C, o dispositivo aberto, os parâmetros seriais e a exclusividade; demonstrar que não há segundo leitor TTY ou Agent serial concorrente.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** configuração CH340 via termios e teste de exclusividade.

### REQ-NF-07 — Escrita USB no Ciclo de Simulação

A escrita das saídas destinadas à DAQC deve ocorrer ao final do ciclo de simulação correspondente, preservando a ordem temporal entre aquisição de entradas, processamento da FMU e publicação das saídas.

**Critério de aceitação proposto (V-NF-07; HOST + HIL):** Instrumentar ciclo n e demonstrar escrita do conjunto de saídas correspondente depois do processamento e antes do ciclo seguinte, dentro do limite contratado.

**Design:** ARCH-TIME / IF-DAQ. **Execução:** TASK-004, TASK-007. **Pendência:** fronteira de entrega e orçamento medidos.

### REQ-NF-08 — Proteção de Dados Compartilhados

O compartilhamento de dados entre as threads de leitura USB e de simulação deve utilizar double-buffering, com mutex restrito à troca/seleção do buffer e às regiões críticas necessárias.

**Critério de aceitação proposto (V-NF-08; HOST + alvo):** Sob escritor/leitor concorrentes, snapshot nunca mistura gerações; mutex limitado à troca/cópia necessária e sem I/O sob lock. Medir bloqueio máximo aprovado.

**Design:** ARCH-IO / IF-SAMPLE. **Execução:** TASK-004, TASK-007. **Pendência:** implementação e medição do bloqueio máximo.

### REQ-NF-09 — Atualização da Interface Gráfica

Atualização visual dos gráficos deve ocorrer no máximo a 10 Hz, desacoplada do ciclo HiL, sem obrigar perder amostras de logging ou reduzir a frequência do núcleo.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-09; GUI + alvo):** Contar atualizações por janela durante run e verificar teto de 10 Hz; atraso de GUI pode reduzir renderização sem bloquear simulação; histórico respeita janela/vida do gráfico.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** medição de taxa e interferência.

### REQ-NF-10 — Prioridade da Interface Gráfica

A interface gráfica deve ser executada em thread distinta das threads críticas de simulação e comunicação, utilizando prioridade não real-time ou inferior às threads críticas.

**Critério de aceitação proposto (V-NF-10; HOST + alvo):** Identificar thread GUI distinta e política não RT/inferior à crítica; bloquear atualização em ensaio não desloca ciclo além do critério temporal aprovado.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** ensaio de carga no alvo.

### REQ-NF-11 — Linguagem do Núcleo de Simulação

As funções responsáveis pelo núcleo de simulação, gerenciamento da FMU e execução da thread de simulação devem ser implementadas em linguagem C.

**Critério de aceitação proposto (V-NF-11; HOST):** Inspeção/build comprovam fontes C do núcleo/wrapper/thread; API GUI/ROS não transfere a execução FMU para código de interface.

**Design:** ARCH-CORE. **Execução:** TASK-001. **Pendência:** Nenhuma.

### REQ-NF-12 — Interface Gráfica

GUI deve utilizar Qt Quick/QML em Qt 6/C++, desacoplada do núcleo C, que pode operar pelo terminal em modo debug. A aparência deve lembrar Linux Mint; APIs Qt são preservadas e estilo próprio segue constituição.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-12; HOST + GUI):** Executar cenário pelo núcleo sem GUI e pela GUI; conferir controles/cor e baixa prioridade, sem dependência de widgets no núcleo. Cores específicas são design proposto, não tokens oficiais Mint exigidos.

**Design:** ARCH-GUI. **Execução:** TASK-009. **Pendência:** implementação do estilo e revisão visual.

### REQ-NF-13 — Biblioteca FMI

A manipulação de FMUs 2.0 deve utilizar a biblioteca FMILibrary (FMILib) ou camada de abstração construída sobre ela.

**Critério de aceitação proposto (V-NF-13; HOST):** Identificar versão FMILibrary vinculada; wrapper executa fixture por essa biblioteca/camada; não usar binário legado como evidência da dependência atual.

**Design:** ARCH-CORE. **Execução:** TASK-001. **Pendência:** versões/arquiteturas das dependências e fixtures.

### REQ-NF-14 — Biblioteca de Abstração para FMU

O projeto deve possuir uma biblioteca complementar à FMILibrary para encapsular operações recorrentes, incluindo importação e destruição de FMUs, tratamento de logs/erros e identificação dos tipos das variáveis de entrada e saída.

**Critério de aceitação proposto (V-NF-14; HOST):** Camada encapsula importar/destruir, erros e tipos de inputs/outputs; falha em cada fase libera recursos e permite próxima execução válida.

**Design:** ARCH-CORE / IF-CORE. **Execução:** TASK-003, TASK-006. **Pendência:** matriz de capacidades FMI e testes de incompatibilidade.

### REQ-NF-15 — Arquitetura Modular

O software deve ser estruturado em módulos separados, no mínimo, para interface gráfica, núcleo de simulação/FMU e comunicações USB/ROS 2, reduzindo acoplamento entre subsistemas.

**Critério de aceitação proposto (V-NF-15; HOST):** Inspecionar dependências e executar núcleo sem GUI/ROS; componentes de comunicação e GUI usam interfaces sem chamar a instância FMU diretamente.

**Design:** ARCH-CORE / ARCH-IO / ARCH-GUI. **Execução:** TASK-003, TASK-007, TASK-009. **Pendência:** interfaces internas e testes de build modular.

### REQ-NF-16 — Extensibilidade de Perfis de DAQC

Características DAQC devem ser modulares por perfil; o primeiro chama-se ESP32 e disponibiliza ADC/DAC internos, digital e PWM, com reservas UART e exclusão de funções incompatíveis por pino. O mapa analógico trabalha em volts; código DAC 0…255 é conversão de representação, não faixa em volts. Novos perfis não exigem alteração extensiva do núcleo.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-16; HOST + bancada):** Catálogo identifica capacidades/exclusões; não permitir dois usos incompatíveis de um pino. Dois perfis sintéticos testam extensibilidade e perfil real é verificado separadamente.

**Design:** ARCH-IO / IF-CORE. **Execução:** TASK-006, TASK-008. **Pendência:** caracterização dos canais e validação em bancada.

### REQ-NF-17 — Aplicação da DAQC em micro-ROS

A aplicação executada no ESP32 deve utilizar micro-ROS, com mensagens e tópicos compatíveis com a interface ROS 2 da main board.

**Critério de aceitação proposto (V-NF-17; Bancada + HIL):** Firmware do ESP32 informado comunica por micro-ROS com host Humble usando mensagens compatíveis; demonstrar o agente/transporte definido em Q-01, não apenas publicação por bridge de frames próprios.

**Design:** ARCH-FW / IF-DAQ. **Execução:** TASK-008. **Pendência:** ESP-IDF/FreeRTOS/micro-ROS escolhidos e medição em placa.

### REQ-NF-18 — Organização dos Ambientes ROS

Os componentes ROS 2/micro-ROS da main board e da DAQC devem ser mantidos em diretórios próprios e isolados do núcleo principal da aplicação, quando aplicável.

**Critério de aceitação proposto (V-NF-18; HOST):** Pacotes host/firmware/mensagens ficam em diretórios próprios e não impõem headers ROS ao núcleo independente; build separado exercitado.

**Design:** ARCH-IO. **Execução:** TASK-001, TASK-007. **Pendência:** estrutura de build após definição dos componentes.

### REQ-NF-19 — Compatibilidade das Mensagens de I/O

As mensagens de comunicação com a DAQC devem representar a quantidade e os tipos de I/O suportados pelo perfil do ESP32/DAQC, incluindo entradas e saídas digitais e analógicas.

**Critério de aceitação proposto (V-NF-19; HOST + HIL):** Mensagem representa cada canal habilitado e tipo do perfil sem omissão/troca de direção; receptor rejeita perfil ou versão incompatível.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-006, TASK-007, TASK-008. **Pendência:** vetores por direção e firmware.

### REQ-NF-20 — Codificação Numérica

Valores reais de 32 bits transmitidos pelo protocolo devem utilizar representação IEEE 754 float32.

**Critério de aceitação proposto (V-NF-20; HOST + bancada):** Vetores conhecidos demonstram float32 IEEE 754 e byte order contratado; zeros, extremos e valores não finitos seguem política definida, não memcpy dependente do host.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** vetores e conversão portável.

### REQ-NF-21 — Estrutura do Protocolo por Perfil

A organização das variáveis no PAYLOAD USB deve ser definida pelo perfil da DAQC, de forma determinística e documentada, permitindo que main board e DAQC interpretem o mesmo layout.

**Critério de aceitação proposto (V-NF-21; HOST + HIL):** Dado perfil versionado, host e DAQC produzem/interpretam layout idêntico, offsets/tamanhos explícitos; versão errada é rejeitada.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** gerador/validador de schema e teste de hash.

### REQ-NF-22 — Campos de Sincronização do Protocolo

Protocolo deve usar SYNC fixo de dois bytes 0x7259 e MID de um byte: CONFIG=0x01, DATA=0x02, READ_ACK=0x03 e XRCE=0x04. Ordem little-endian adotada do RaspDAQ: SYNC no fio 59 72, independente da arquitetura do processador.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-22; HOST + bancada):** Vetores usam SYNC e MIDs exatos; conferir ordem 59 72, READ_ACK de 5 bytes e demultiplexação XRCE. BB77 permanece somente na referência histórica.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** implementação e vetores READ_ACK/XRCE.

### REQ-NF-23 — Validação de Frames

O receptor deve validar SYNC, MID, tamanho determinado pelo tipo/schema, estado e SEQ quando aplicável antes de interpretar COMMAND, STATUS ou PAYLOAD. O protocolo não usa CRC.

**Critério de aceitação proposto (V-NF-23; HOST + bancada):** Campos estruturais inválidos nunca levam à aplicação de comando/payload; testar fragmentos, ruído, recuperação de fronteira e alteração de bits ainda plausível. Esta última pode não ser detectada sem CRC e a evidência deve registrar o limite.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** parser acumulador e testes sem CRC.

### REQ-NF-24 — Comandos do Protocolo

COMMAND possui um byte: 0x01 DISABLE, 0x02 ENABLE (IDLE), 0x03 STREAMING. Comandos de configuração devem continuar sendo interpretados durante streaming, inclusive DISABLE.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-24; HOST + HIL):** Exercitar três comandos e comandos inválidos; DISABLE em carga interrompe streaming e mantém recepção disponível. STATUS não é definido por inferência de MID.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007, TASK-008. **Pendência:** implementação e ensaio de prioridade CONFIG.

### REQ-NF-25 — Tamanho máximo do payload de dados

PAYLOAD DATA deve ter no máximo 256 bytes, usando somente a quantidade definida pelo perfil. Esse limite não inclui SYNC, MID e SEQ; o quadro DATA pode ter até 261 bytes.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-25; HOST + bancada):** Testar DATA com N=0, N=256 e excesso; quadro máximo de 261 bytes. Parser dimensiona pelo schema, sem overflow nem depender de um read por frame.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-007. **Pendência:** teste dos limites N=0/256.

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

Núcleo deve medir perdas e continuar sem compensação: manter grade fixa de liberações do relógio real, h e sequência da FMU. Após ultrapassar uma liberação, aguardar próximo instante fixo; nunca saltar etapa FMI nem executar rajadas para recuperar. Após a execução, informar deadlines de etapas perdidos, instantes da grade não utilizados e pior atraso de entrega, sem confundir instantes perdidos com etapas FMI omitidas.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-28; HOST + alvo):** Clock controlado com h=10 ms e etapa 0→12 ms: próximo início em 20 ms, h inalterado e sequência FMI contínua; uma entrega atrasada, uma liberação perdida e worst case 2 ms. Testar término exato no limite e atraso de várias liberações; nenhum print temporal no ciclo.

**Design:** ARCH-TIME. **Execução:** TASK-004. **Pendência:** fronteira de entrega e tolerâncias de ensaio.

### REQ-NF-29 — Detecção de Timeout USB

Cada transferência USB deve ter timeout positivo de no máximo 5 ms. Descartar pacote atrasado e usar último dado válido disponível na entrega do passo, reutilizável até fim da execução; contar timeouts. Valor constante não é dado inválido nem evidência de host morto. A simulação não espera indefinidamente pelo enlace. Identificação/expiração de pacote no fio permanece Q-06; streaming sem leitura durante 60 s segue F-27.

**Atualização 0.2:** respostas do usuário registradas em ADR-002; detalhes não resolvidos permanecem explícitos.

**Critério de aceitação proposto (V-NF-29; HOST + HIL):** Injetar atraso e verificar configuração e tempo observado; testar limites positivos até 5 ms, rejeitar infinito/timeout sem limite. Registrar que configuração de API não prova limite sob scheduler.

**Design:** ARCH-IO / IF-DAQ. **Execução:** TASK-004, TASK-007. **Pendência:** medição da latência fim a fim e do timeout.

### REQ-NF-30 — Isolamento de Falhas da Interface

Falhas ou atrasos na atualização da interface gráfica e na plotagem não devem bloquear nem alterar a cadência do loop principal de simulação.

**Critério de aceitação proposto (V-NF-30; HOST + alvo):** Atrasar/parar consumidor gráfico em ensaio com carga; ciclo não executa I/O de GUI nem espera por ela; medir cadência contra tolerância aprovada.

**Design:** ARCH-LOG / ARCH-GUI. **Execução:** TASK-002, TASK-004, TASK-009. **Pendência:** orçamento de custo por cenário.

### REQ-NF-31 — Consistência Temporal do Ciclo

Cada ciclo de simulação deve consumir um conjunto consistente de entradas, executar uma única etapa da FMU e publicar um conjunto correspondente de saídas antes do início do ciclo seguinte.

**Critério de aceitação proposto (V-NF-31; HOST + HIL):** Cada ciclo tem snapshot consistente, um doStep e publicação do conjunto correspondente antes do seguinte; injetar atualização concorrente, stale e escrita tardia e observar política contratada.

**Design:** ARCH-TIME / IF-SAMPLE. **Execução:** TASK-004, TASK-006, TASK-007. **Pendência:** testes concorrentes e temporais.

### REQ-NF-32 — Meta de desempenho de pelo menos 100 Hz

A plataforma deve ter como meta executar simulações a pelo menos 100 Hz (passo de referência de 10 ms), configuráveis, sem depender de uma planta fixa. Cumprimento deve ser medido para o modelo/host/perfil ensaiado; importar qualquer FMU 2.0 CS não garante que todo modelo caiba nesse orçamento.

**Origem 0.2:** comportamento novo solicitado nas respostas DEC; ID acrescentado sem renumerar requisitos originais.

**Critério de aceitação proposto (V-NF-32; HOST + HIL):** Ensaiar fixtures documentadas a 100 Hz, medir ciclo completo, perda/pior atraso e comunicação. Informar duração/carga e critérios de desempenho pendentes, sem aprovar meta só por configurar h=0,01.

**Design:** ARCH-TIME. **Execução:** TASK-004, TASK-010. **Pendência:** modelo/carga/ambiente e limites de aceitação medidos.

## Critérios transversais e lacunas

Implementação não iniciada. Respostas 1…5 e a política de comunicação estão consolidadas: ausência separada, último snapshot na inserção, zero no fim/restart inicializado, ADC/PWM configuráveis, grade fixa sem compensação, DATA sem CRC/retransmissão e READ_ACK separado. Restam versões reproduzíveis, parâmetros medidos de transporte e critérios de bancada.

Suporte geral FMI exige distinguir metadados, capacidades e binário de execução da meta de desempenho. O log é de outputs da FMU; retenção/counter de aquisição é nos inputs da FMU. A ausência de histórico válido não autoriza inventar zero. F-27 não exige zerar saídas físicas. Valores didáticos não definem limites do produto.

## Histórico de trabalho

| Revisão | Alteração | Natureza |
|---|---|---|
| 0.1 | 53 requisitos importados; NF-26/27 revisados; critérios propostos | Instrução inicial e ADR-001 |
| 0.2 | Respostas DEC aplicadas, cinco IDs derivados, novos SYNC/limite payload/teto gráfico/log | Instrução explícita do usuário e ADR-002 |
| 0.2 | Lacunas Q mantidas, sem implementação nem aprovação de testes | Consolidação documental em andamento |
| 0.3 | Q incorporadas, boot zero, Error por saída, log bruto/aviso, timeout por transferência, PWM e novo F-27; inspeção RaspDAQ | Documentação; sem implementação ou ensaio físico |
| 0.4 | Correção da direção de aquisição/atuação; retenção no host, 100 passos por canal, 60 s sem leitura e uso dos dois núcleos | Instrução explícita; detalhes de mecanismo ainda propostos |

| 0.5 | Última atualização na inserção FMI, ausência separada, zero físico no fim/restart pela FMU, ADC/PWM configuráveis e proibição de compensação temporal | Respostas 1…5 e grade fixa confirmada |
| 0.6 | Reuso RaspDAQ: little-endian, CONFIG 4/5, STATUS somente CONFIG, SEQ/schema fixo, estados/ownership; limites e extensões identificados | Instrução de usar a referência e ADR-003; sem teste de produto |
| 0.7 | Comunicação real-time sem CRC ou retransmissão; gap segue para o próximo DATA; READ_ACK MID 03 e XRCE MID 04; sessão no fio removida | Instrução explícita do usuário e fechamento do ICD |
