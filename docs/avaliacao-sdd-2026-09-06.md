# Avaliação do MICROHIL e do estudo MICROHIL-IO-Demo

Data: 06.09.2026. Status: avaliação técnica e proposta de evolução; não aprova nem altera requisitos. Código principal inspecionado: `c788a4283cf81f17e9a2956ae258487c7931590a`.

O `microHiL` possui uma base modular de execução de FMU em C que merece ser preservada. Ainda é um protótipo de simulação no host: não há implementação da malha física DAQC, integração ROS ou interface Qt. O `MICROHIL-IO-Demo` oferece um exemplo documental de SDD, sem firmware ou ensaios executados. Sua estrutura pode orientar o desenvolvimento, mas suas decisões e números não pertencem à especificação do produto.

O passo imediato é tornar o protótipo reproduzível e verificável, enquanto se resolvem as divergências de contrato. Não é necessário reescrever o núcleo ou copiar os 15 arquivos do Demo.

## 1. Fontes, inventário e limites

Foram lidos os sete arquivos C, sete headers, CMake e README do principal; o conteúdo textual e as tabelas de `MICROHIL-REQ-001-A.docx`; os metadados XML e cabeçalho ELF da FMU; os 15 Markdown do Demo; e o histórico Git disponível. Não foram encontrados `AGENTS.md` nas pastas ou ancestrais consultados. `.agents/` e `.codex/` estão vazios nas duas pastas. O `SKILL.md` do Demo é um procedimento didático local, sem autoridade sobre os requisitos do principal.

| Elemento | Situação observada | Consequência |
|---|---|---|
| Histórico principal | Dois commits; último adiciona a FMU | Mensagem de commit não é evidência de teste |
| `src/`, `include/` | Configuração, menu, FMU, simulação, fila, CSV, plotagem | Separação inicial aproveitável |
| `build/` | Executável, objetos e cache versionados; caminhos `/home/carla/...` | Não comprova build nesta máquina; separar fontes de artefatos gerados |
| Documento REQ | Revisão A, versão 01, 24.08.2026; 22 funcionais e 31 não funcionais | Fonte documental disponível; aprovação/vigência não comprovadas pelo histórico |
| `docs/` inicial | Não rastreado no Git (`?? docs/`) | O REQ está na pasta, mas ainda não compõe a história versionada |
| CONOPS, apresentação, três PDFs metodológicos | Não encontrados nestas pastas | Usado o resumo fornecido; nenhuma alegação de releitura dos PDFs |
| Testes do principal | Nenhuma suíte, registro CTest, CI ou evidência de ensaio encontrada antes desta avaliação | Implementação observada não equivale a requisito validado |
| Demo | 15 Markdown; 11 FR e 7 NFR; um registro fictício | Nenhum firmware, build ou ensaio real encontrado |

A leitura do DOCX foi semântica por OOXML; não foi uma revisão visual de sua diagramação. Os ensaios desta avaliação estão em [evidence/audit-2026-09-06/README.md](evidence/audit-2026-09-06/README.md). Nenhum hardware foi acionado; não houve execução de FMU, instalação de dependências, alteração de privilégios ou uso do executável preexistente como prova do código atual.

## 2. Relação com a passagem de contexto

| Intenção/contexto | Fonte efetiva e situação | Encaminhamento |
|---|---|---|
| MICROHIL evolui para PIHIL | Contexto do usuário e Demo; principal ainda se apresenta genericamente como runner | Registrar identidade e fronteiras no README/contexto, sem antecipar implementação PIHIL |
| Raspberry Pi/Linux de baixo custo | REQ-NF-01 exige Ubuntu 22.04; README menciona Raspberry Pi como alvo possível | Placa, arquitetura, kernel, orçamento e critérios de custo ainda precisam de confirmação |
| Modelica → FMU 2.0 CS | Loader verifica FMI 2.0 e CS; FMU declara geração por OpenModelica | Preservar wrapper; adicionar fixtures e ensaios reproduzíveis |
| Núcleo C, thread prioritária | Implementado com pthread e tentativa SCHED_FIFO | Fallback padrão permite scheduler normal; definir conformidade com REQ-NF-03 |
| ROS 2 Humble e micro-ROS/ESP32 | REQ-NF-02 e 17; sem implementação | Não inferir transporte da simples presença desses requisitos |
| USB Bulk/libusb versus micro-ROS | REQ-F-18/19/20 e NF-04 a 08, 20 a 25 coexistem com NF-17 | Registrar ADR com camadas, participantes e coexistência ou substituição explícita |
| GUI desacoplada | REQ-NF-12 exige Qt 6/C++; código usa terminal e gnuplot | Implementação atual não comprova mudança aprovada para outra GUI |
| CSV solicitado na conversa | Implementado; REQ-F-04/10 prescrevem arquivos binários para perfil/log | Decidir CSV, binário ou ambos; atualizar somente requisitos afetados |
| Entradas e saídas, mapeamento | Código seleciona somente outputs para plot/CSV | Seleção de saída não é mapeamento de I/O físico |
| Timing deve ser medido | Há métricas parciais e advertência correta no README | Instrumentação atual ainda não sustenta aceitação temporal |

USB Bulk é um tipo de transferência, libusb é uma API no host, e micro-ROS define outra camada de comunicação. O problema documental é a ausência da arquitetura que explique como essas exigências se relacionam. Não há evidência suficiente para declarar que são incompatíveis em qualquer arquitetura, nem que o USB CDC do Demo resolve o contrato do produto.

O documento exige no mínimo `Idle`, `Running`, `Error`, `Stopped` e `Finished`. A pausa aparece apenas como ponto de avaliação no contexto: deve ser uma decisão explícita, não um requisito inventado. Os estados de segurança embarcados do Demo não substituem os estados da aplicação host.

## 3. Arquitetura efetivamente implementada

`main.c` mantém configuração e modelo, importa a FMU, escolhe saídas, cria consumidores e inicia a simulação. Durante a execução aguarda `pthread_join`; portanto não há menu operacional concorrente para editar controles ou pausar. SIGINT/SIGTERM solicitam parada por flag atômica, consultada pelo ciclo.

`rt_simulation.c` inicializa, executa e termina a instância FMU. Cada passo chama `doStep`, lê saídas, espera um instante absoluto monotônico e copia a amostra para duas filas independentes. O tempo final pode encurtar o último passo. `csv_logger.c` grava uma fila e `plotter.c` consome a outra e envia dados ao processo gnuplot. Não existe etapa de leitura DAQC, aplicação de entradas FMU ou publicação de saídas ao hardware.

| Dado | Escritor/proprietário observado | Leitor e mecanismo |
|---|---|---|
| `AppConfig` | Menu antes da execução | Threads recebem ponteiro const; menu fica bloqueado durante a execução |
| Instância FMU | Simulação durante execução; importação/destruição coordenadas por main | Sem callbacks ROS ou GUI acessando a instância |
| Amostra | Simulação | Cópia por fila SPSC independente para cada consumidor |
| Índices e descartes | Produtor/consumidor conforme fila | Atomics acquire/release; slot reservado resulta em 4095 amostras úteis |
| Pedido de parada | Handler de sinal | Flag atômica lida pela simulação |
| Fim de produção | Simulação ou main quando falha a criação da thread | Flag atômica lida pelos consumidores |
| Estatísticas | Simulação | Main lê após join |

Essa separação é compatível com a intenção de desacoplamento, mas ainda faltam contratos explícitos de estados, erros, entrada de comandos, validade, backlog e sobrecarga. A ausência de mutex na fila não prova que todos os atomics são lock-free em qualquer alvo; isso precisa ser verificado no target escolhido.

## 4. Matriz dos 53 requisitos disponíveis

Legenda: **I** = mecanismo identificado no código, sem aceitação integral executada; **P** = parcial; **A** = ausente; **D** = divergência entre documento e implementação; **NA** = sem componente ao qual aplicar neste estado. Nenhuma linha significa aprovação de produto. Referências a arquivos nesta tabela são relativas à raiz principal.

| Requisito | Estado | Evidência de implementação ou lacuna |
|---|---|---|
| REQ-F-01 | I | `fmu_model.c:36` verifica FMI 2.0, CS, XML e carregamento do binário; sem execução nesta avaliação |
| REQ-F-02 | A | Sem mapeamento FMU↔DAQC; seleção em `main.c` é para CSV/plot |
| REQ-F-03 | A | Sem perfil ou habilitação DAQC |
| REQ-F-04 | A | Configuração somente em memória; sem perfil binário |
| REQ-F-05 | P | `main.c:146` configura passo/duração via terminal; validação insuficiente |
| REQ-F-06 | P | Menu inicia execução; sem Play gráfico e validação completa |
| REQ-F-07 | P | Flag de parada por sinal; sem Stop gráfico e garantia de finalização de logging |
| REQ-F-08 | A | Sem modelo explícito dos cinco estados exigidos |
| REQ-F-09 | P | Mensagens em stderr; falhas de consumidores não compõem resultado agregado |
| REQ-F-10 | D | CSV habilitável antes do início; sem arquivo binário ou GUI |
| REQ-F-11 | P | Timestamp, sequência e valores no CSV; fila pode descartar amostras |
| REQ-F-12 | A | Métricas agregadas somente no terminal; CSV não inclui métricas requeridas |
| REQ-F-13 | P | Passos, misses, máximo de processamento e atraso parcial; faltam médias e USB |
| REQ-F-14 | P | `plotter.c:45` plota todas as variáveis no mesmo gráfico; sem janela por variável |
| REQ-F-15 | P | Janela temporal e refresh configuráveis; faltam limites Y e definição de resolução |
| REQ-F-16 | A | Não há controle de atualização durante execução |
| REQ-F-17 | A | Sem entradas virtuais ou setters FMU |
| REQ-F-18 | A | Sem frames de configuração USB |
| REQ-F-19 | A | Sem streaming USB |
| REQ-F-20 | A | Sem comandos/STATUS DAQC |
| REQ-F-21 | P | Verifica FMU e ao menos uma saída; aceita valores temporais não finitos |
| REQ-F-22 | P | Tenta drenar/fechar CSV; erro de escrita oculto e corrida de fim de produção |
| REQ-NF-01 | P | Fontes POSIX/Linux; componentes compilados em Ubuntu 22.04; build integral bloqueado |
| REQ-NF-02 | A | Sem ROS 2 Humble no código/CMake |
| REQ-NF-03 | P | Thread dedicada e SCHED_FIFO; fallback permitido por padrão |
| REQ-NF-04 | A | Sem thread de leitura USB |
| REQ-NF-05 | A | Sem transferências Bulk |
| REQ-NF-06 | A | Sem libusb |
| REQ-NF-07 | A | Sem escrita DAQC ao final do ciclo |
| REQ-NF-08 | A | Sem buffers de entradas USB; filas de telemetria não satisfazem esse contrato |
| REQ-NF-09 | D | Refresh padrão 0,10 s, mas configurável e por polling; não fixa 10 Hz |
| REQ-NF-10 | P | Menu e plot separados da simulação; sem GUI Qt nem teste de prioridade/carga |
| REQ-NF-11 | I | Núcleo, FMU e simulação implementados em C |
| REQ-NF-12 | D | Terminal/gnuplot em lugar da GUI Qt 6/C++ prescrita |
| REQ-NF-13 | I | Wrapper e CMake usam FMILibrary; dependência não resolvida neste ambiente |
| REQ-NF-14 | P | `fmu_model.c` encapsula ciclo de vida; não enumera/aplica entradas |
| REQ-NF-15 | P | Módulos atuais separados; comunicações e GUI requeridas ausentes |
| REQ-NF-16 | A | Sem abstração de perfis DAQC |
| REQ-NF-17 | A | Sem aplicação ESP32/micro-ROS |
| REQ-NF-18 | A | Sem componentes ou diretórios ROS |
| REQ-NF-19 | A | Sem mensagens de I/O por perfil |
| REQ-NF-20 | A | Sem serialização float32 de comunicação; doubles internos não atendem isso |
| REQ-NF-21 | A | Sem layout de PAYLOAD por perfil |
| REQ-NF-22 | A | Sem SYNC BB77/MID |
| REQ-NF-23 | A | Sem parser/validação de frames |
| REQ-NF-24 | A | Sem comandos Disable/Enable/Start Streaming |
| REQ-NF-25 | A | Sem limite de frame implementado |
| REQ-NF-26 | D | Arquivos snake_case; funções e variáveis também snake_case, divergindo do REQ |
| REQ-NF-27 | NA | Sem componente Python de produto |
| REQ-NF-28 | P | Mede doStep+leitura e atraso absoluto antes do sleep; não mede todo ciclo |
| REQ-NF-29 | A | Sem timeout USB configurável |
| REQ-NF-30 | P | Filas evitam escrita direta de gráficos no ciclo; falta teste sob falha/carga |
| REQ-NF-31 | P | Um doStep e leitura por ciclo, mas sem entradas e saída DAQC consistentes |

## 5. Achados priorizados

### A01 — Alta: falhas de logging podem parecer sucesso

Em `src/csv_logger.c:28`, `32`, `42` e `43`, retornos de escrita/flush/close não são verificados; o resultado é zero ao terminar. Ensaio HOST com `/dev/full` reproduziu retorno zero apesar da falha de escrita. Além disso, `src/main.c:235` ignora resultados dos joins de CSV e plot; o `Result: OK` depende apenas da simulação. Impacta REQ-F-09/11/22. Corrigir propagação, diagnóstico e classificação de execução incompleta, com falha de abertura e escrita testadas.

### A02 — Alta: encerramento do CSV tem uma janela de corrida

Inspeção de `src/csv_logger.c:27–38`: consumidor encontra fila vazia; produtor pode publicar a última amostra e marcar `producer_done`; consumidor lê true e sai sem nova leitura da fila. Portanto a última amostra pode ficar pendente. O plotter já faz uma segunda leitura em condição semelhante. Achado por interleaving estático, não reproduzido dinamicamente nesta avaliação. Criar ensaio determinístico controlando esse interleaving, junto ao caso de parada/erro.

### A03 — Alta: métricas temporais não representam todo o contrato

`src/rt_simulation.c:79–116` mede doStep+leitura, compara o relógio com deadline antes de dormir e não recalcula atraso de liberação após o despertar. Também exclui publicação nas filas do tempo computacional. Um atraso de scheduler pode não aparecer no contador daquele ciclo. Isso não significa que o contador seja inútil: ele mede uma condição mais estreita do que o rótulo sugere. Separar duração de processamento, duração do ciclo, atraso de liberação e deadline de disponibilidade das saídas. Definir tolerâncias e política de overrun antes de aprovar REQ-NF-28/31. Não usar o máximo observado como prova de limite universal.

### A04 — Alta: valores não finitos passam pela configuração

`src/main.c:44–52`, `146–150` usa `strtod` e rejeita somente `<= 0`. `NaN` passa nessa comparação; duração NaN faz a condição do loop falhar e permite terminar com zero passos e sucesso. Infinito e overflow também não são validados, assim como alguns parâmetros de plotagem. Achado estático. Testar NaN, ±Inf, overflow, valores extremos, passo maior que duração e limite representável antes de iniciar threads.

### A05 — Alta para evolução: falta contrato de comunicação e estado seguro

Não existem implementação, ICD do produto ou ADR que resolvam Bulk/libusb e micro-ROS. Faltam comprimento/layout exato por perfil, ordem dos bytes, valores de MID/COMMAND/STATUS, sequência/sessão, integridade, dados atrasados, timeout, reconexão, estados e confirmação de configuração. BB77, float32 e limite de 256 bytes isoladamente não fecham o contrato. Nenhum valor do Demo deve preencher essas lacunas automaticamente. Ausência de dados físicos bloqueia o driver/ensaio afetado, mas não as correções HOST.

### A06 — Média: documentação promete isolamento mais forte que o demonstrado

O README afirma que a thread RT nunca faz I/O de arquivo. Há `fprintf(stderr, ...)` nos caminhos de falha do wrapper chamados dentro do ciclo (`src/fmu_model.c:184–228`) e callbacks de logging da biblioteca/FMU. Inicialização também ocorre depois de ativar SCHED_FIFO. Preservar CSV/plot fora do ciclo, mas limitar a afirmação ao que foi verificado e mover diagnósticos críticos para registros limitados consumidos fora dele. Alocações internas e WCET da FMU não foram auditados.

### A07 — Média: parada, estados e erros de relógio incompletos

O menu aguarda join; não há pausa, FSM ou comandos durante execução. A flag de parada não interrompe uma FMU bloqueada; a latência pode incluir doStep e espera restante. Erros de `clock_nanosleep` diferentes de EINTR não são tratados. Término natural e parada compartilham resultado zero. Definir transições, timeout de parada e política para FMU bloqueada; não prometer encerramento limitado sem mecanismo e ensaio.

### A08 — Média: filas e recursos precisam de orçamento do alvo

As duas filas locais em `run_simulation` ocupam aproximadamente 4,25 MiB de pilha no ABI HOST medido (2.228.248 bytes cada). O histórico local de 5000 amostras do plotter soma aproximadamente 2,59 MiB de dados. Não se observou estouro; trata-se de orçamento a verificar no alvo. A fila cheia descarta a amostra nova e só reporta contagem agregada. Para logging de cada instante, é necessário especificar capacidade, perda tolerável e efeito no resultado. Plot e CSV podem ter políticas diferentes.

### A09 — Média: exemplo FMU não cobre entradas nem o Raspberry Pi

O XML declara 14 outputs e nenhum input. O único binário da FMU é ELF64 x86-64 (`e_machine=62`), em `binaries/linux64/`. Não há binário ARM incluído. Logo o arquivo atual não prova execução nativa no Raspberry Pi nem aplicação de entradas vindas da DAQC. Preservá-lo como exemplo de outputs; preparar fixture simples com entradas reais, inteiras e booleanas e export compatível com o alvo confirmado.

### A10 — Média: reprodutibilidade e rastreabilidade incompletas

A configuração CMake limpa falhou por FMILibrary não encontrada. O build versionado aponta para outra máquina; não existe versão fixada da dependência, suíte ou CI encontrada. O DOCX ainda não está rastreado e suas exigências não possuem critérios de aceitação sistemáticos. A convenção de nomes REQ-NF-26 diverge do código, mas renomear tudo agora não corrige os riscos funcionais: decidir sua revisão ou aplicação antes de refatorar.

Outros pontos para testes do wrapper: último passo reduzido sem checagem de capacidade de passo variável da FMU importada; valores de saída não finitos; nomes truncados; escaping de nomes em CSV e comandos gnuplot. A FMU de exemplo declara capacidade de passo variável, mas isso não cobre todas as FMUs aceitas pelo loader.

## 6. Como aproveitar e revisar o Demo

O Demo demonstra corretamente as funções de contexto, constituição, alvo, requisitos, design, contrato, ADR, tarefa, teste, evidência e gates. A cadeia `FR-006 → safety_supervisor → TASK-004 → TEST-005 → EV-TEST-005` é explícita. A evidência diz `NÃO EXECUTADO`, contém hash fictício e não aprova o requisito. Todos os gates permanecem desmarcados.

Reaproveitar o método de formular estímulo, resposta, limite e verificação, além dos formatos de tarefa e evidência. Não copiar ESP32-S3, GPIO4/GPIO18, 1 kHz, PWM 20 kHz, timeout 100 ms, consumo, memória, SDK ou QoS para a especificação principal. As restrições de memória do firmware didático também não podem ser aplicadas automaticamente ao processo Linux que carrega FMUs.

Há lacunas didáticas que precisam de refinamento antes de virar implementação:

1. `INT-001` tem sequência, timestamp do host e valor, mas não define identidade de sessão no fio nem handshake que comprove nova geração. Uma geração local, sozinha, não demonstra rejeição de mensagem antiga entregue após reconexão/timeout. O wraparound aparece como teste, mas falta a regra de comparação.
2. `constitution.md` exige estado seguro para comando inválido, enquanto FR-004/contrato enfatizam rejeitar e não renovar timeout. É preciso decidir se invalidação implica saída segura imediata ou manutenção até expiração; as leituras não são equivalentes.
3. Arquitetura atribui estado a `safety_task` e atuação a `control_task`, mas a TASK-004 chama diretamente a HAL PWM no supervisor. Esclarecer escritor final e atomicidade da decisão estado/comando/atuação para evitar reaplicação concorrente.
4. TASK-004 está marcada como pronta, embora suas pré-condições de HAL/clock/FSM não possuam código ou ensaios na pasta. Interpretar como exemplo de tarefa preparada documentalmente, não prontidão real.
5. Plano pede fechar contrato antes do desenvolvimento, mas SDK/mensagens ainda têm detalhes pendentes. Gates precisam considerar evidência e dependências do escopo, sem bloquear trabalho HOST independente por falta de pinagem.

Os limites elétricos e temporais do Demo não foram conferidos contra datasheets nesta avaliação. Isso é consistente com seu caráter didático, mas impede tratá-los como projeto de hardware validado.

## 7. Estrutura documental recomendada sem duplicação

Usar o REQ existente como fonte dos IDs `REQ-F-*` e `REQ-NF-*`; não introduzir os IDs `FR-*` do Demo no produto. O relatório atual é um retrato datado, não uma nova especificação concorrente.

| Necessidade | Aproveitamento proposto |
|---|---|
| Entrada de agentes e regras permanentes | Um `AGENTS.md` curto com ordem de leitura, português nas explicações, inglês em comentários novos e distinção entre fatos/propostas/evidências; constituição separada só se houver conteúdo suficiente |
| Contexto e fronteiras | Expandir README; criar contexto separado apenas se necessário |
| Alvo real | `docs/TARGET.md` com campos confirmados e pendentes, sem copiar valores do Demo |
| Especificação | Manter DOCX e seus IDs; escolher uma única fonte editável normativa se houver futura migração para Markdown |
| Arquitetura | Evoluir o diagrama do README para `docs/architecture.md` com comportamento atual e propostas separados |
| Decisões e ICD | `docs/adrs/` e `docs/contracts/` para decisões transversais e contratos versionados |
| Backlog/plano/tarefas | Um backlog ordenado com links para requisitos e critérios; fichas separadas somente para incrementos em execução |
| Verificação e rastreabilidade | Uma matriz requisito→contrato→tarefa→teste→evidência; plano de teste e gates por ambiente |
| Evidências | Preservar registros datados em `docs/evidence/`, inclusive falhas e limitações |

SDD-processo é a disciplina que conecta esses elementos. O Software Design Document descreve o design. Backlog organiza trabalho; sprint seleciona trabalho; ICD especifica interfaces. Nenhum deles substitui requisito ou ensaio executado.

## 8. Ordem proposta de desenvolvimento e tarefas verificáveis

IDs `AUD-T*` identificam propostas desta avaliação, não tarefas aprovadas nem requisitos novos. Os critérios abaixo são propostas de verificação; limites físicos permanecem pendentes.

| Ordem/tarefa | Escopo e requisitos relacionados | Dependência | Critério de conclusão proposto |
|---|---|---|---|
| AUD-T01 — Consolidar fontes | REQ completo, contexto, vigência, identidade MICROHIL→PIHIL; registrar pendências e orientação local | Responsável confirmar documentos vigentes | Todos os 53 IDs preservados, divergências com dono/status e nenhuma hipótese do Demo promovida |
| AUD-T02 — Build reproduzível | NF-01/11/13; fixar versão FMILibrary, instruções e separar build gerado | Versão/origem da dependência | Checkout limpo configura e compila no host declarado; versões e log preservados; testes descobertos automaticamente |
| AUD-T03 — Corrigir logging | F-09/11/22; A01/A02 | Pode começar sem ROS/placa; usa fontes independentes de FMILib | Testes de falha de abertura/escrita, último item concorrente, stop/erro e resultado agregado; nenhuma falsa indicação de log completo |
| AUD-T04 — Validar configuração e ciclo de vida | F-05/06/07/08/21, NF-14; A04/A07 | Contrato de estados do host; T02 para integração FMU | NaN/Inf/overflow rejeitados antes da execução; transições e repetição de runs testadas; falhas de inicialização e stop verificadas |
| AUD-T05 — Contratar e medir timing | F-12/13, NF-03/28/30/31; A03/A06/A08 | Definição de ciclo/deadline e modo fallback | Relógio controlado verifica atrasos antes/depois de sleep e overrun; métricas preservadas; teste sob carga com limitações explícitas |
| AUD-T06 — Fechar decisões de produto | F-04/10/14/15/16/18/19/20, NF-02/05/06/12/17/26 | Confirmação técnica das alternativas | ADRs para transporte, GUI/logging e convenções; cada requisito afetado atualizado ou mantido explicitamente |
| AUD-T07 — Definir alvo e ICD | F-02/03/18–20, NF-04–08/16–25/29/31 | Hardware real e T06 para contrato de transporte | Perfil com canais/unidades/estado seguro; layout e validade completos; timeout/timing com valores confirmados; revisão host/DAQC |
| AUD-T08 — Entradas FMU e adaptador simulado | F-02/17/21, NF-14/16/31 | Contrato interno; T02/T04; física não necessária para mock | Fixture com entradas e outputs previsíveis; testes de tipos, unidade/escala, mapa inválido e snapshots coerentes; resultado rotulado HOST |
| AUD-T09 — Comunicação real e firmware seguro | F-03/18–20, NF-02/04–08/17–25/29 | T07 e dados de hardware confirmados | Parser/serialização HOST; boot, reset, timeout e reconexão em bancada; comando antigo não reaplicado conforme ICD |
| AUD-T10 — GUI e persistência | F-04–10/14–17, NF-09/10/12/30 | T06 e API estável de estados/comandos | Controles operam durante simulação; arquivos válidos/corrompidos testados; falha/atraso gráfico sob carga sem violar critério temporal aprovado |
| AUD-T11 — Integração HIL | Requisitos aplicáveis do conjunto | T05/T08/T09 e alvo instrumentado | Malha física, dados inválidos/antigos, perda de link, reset e carga ensaiados; dados brutos, versões e resultados associados aos requisitos |

T02/T03 e preparação de testes HOST podem avançar enquanto T01/T06/T07 aguardam decisões. T08 pode começar com interface interna simulada, sem alegar validação do transporte futuro. Sprints devem selecionar incrementos dessa ordem conforme capacidade da equipe; não há base para inventar duração ou data de entrega.

## 9. Decisões pendentes a levar à equipe

- Qual a vigência/aprovação do REQ-A frente às decisões posteriores do ChatGPT? Onde estão CONOPS e apresentação para futura reconciliação?
- Bulk/libusb permanece obrigatório? Qual o papel do micro-ROS e do agente, e quais camadas coexistirão no mesmo dispositivo?
- Qt 6 permanece a GUI do produto? CSV substitui o logging binário ou é uma exportação adicional? Perfil binário continua requerido?
- Qual Raspberry Pi/arquitetura/SO/kernel e qual placa ESP32, revisão, circuitos ADC/DAC/PWM, pinagem, unidades e faixas reais?
- Quais frequência, jitter, deadline, timeout, dados antigos toleráveis e estados seguros derivam da planta e da malha real?
- O que caracteriza sucesso, execução degradada e erro diante de overrun, fallback de scheduler, perda de log e desconexão?

Essas decisões não impedem concluir a avaliação nem corrigir o logger ou preparar testes HOST. Impedem assumir contratos físicos e declarar validação HiL. A base existente deve evoluir por incrementos pequenos, vinculados aos IDs já presentes e sustentados por evidência observada.
