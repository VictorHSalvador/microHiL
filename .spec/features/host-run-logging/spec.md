# Spec: Logging binário e conversão pós-execução no HOST

> feature: host-run-logging
> status: pronta

## Contexto

O protótipo grava CSV durante a simulação, oculta falhas de escrita e não possui o formato binário tipado definido pelo IF-LOG. Esta feature executa a TASK-002 do plano: substitui o consumidor CSV em tempo de execução por um sink binário assíncrono, preserva as amostras aceitas até o encerramento, torna falhas e perdas observáveis e disponibiliza conversão para CSV somente depois de o registro ser fechado. A fonte normativa dos requisitos do produto continua sendo `docs/spec.md`, especialmente REQ-F-07, REQ-F-09, REQ-F-11, REQ-F-12, REQ-F-22, REQ-F-25, REQ-F-26 e REQ-NF-30.

## Histórias

### US-007 — Preservar outputs da execução em formato tipado

Como responsável pela análise da simulação, quero que cada passo aceito pelo logger seja gravado em um formato binário versionado e vinculado à FMU, para interpretar os outputs sem depender de layout de memória ou de um modelo diferente.

#### AC-015 — O cabeçalho identifica o formato, a FMU e as saídas

- **Dado** um descritor válido com hash SHA-256 da FMU, tempo inicial, passo e saídas selecionadas
- **Quando** um novo registro é aberto
- **Então** o cabeçalho little-endian contém `MHILLOG1`, versão 1, tamanho verificável, hash, tempos e as saídas em índice XML crescente, preservando índice, valueReference, tipo e nome UTF-8

#### AC-016 — Cada passo possui um registro tipado e sem métricas de desempenho

- **Dado** um conjunto de outputs Real, Integer, Enumeration e Boolean com qualidades válidas e inválidas
- **Quando** a amostra final de um passo é serializada e lida novamente
- **Então** sequência, tempo simulado, bitmap de qualidade e valores retornam na ordem do cabeçalho; Real usa float64, Integer/Enumeration int32, Boolean uint8, inválidos seguem IF-LOG e nenhuma métrica temporal de desempenho integra o registro

#### AC-017 — Entradas malformadas são rejeitadas sem alocação ilimitada

- **Dado** cabeçalho, contagem, nome, tipo, bitmap ou registro truncado/inconsistente
- **Quando** o leitor valida o arquivo
- **Então** ele limita tamanhos, detecta overflow ou inconsistência e retorna diagnóstico específico sem interpretar bytes incompletos como uma amostra válida

### US-008 — Isolar persistência e falhas do ciclo de simulação

Como responsável pelo núcleo HiL, quero enfileirar uma cópia limitada de cada amostra e delegar o disco a um consumidor, para que escrita, flush, fechamento e diagnóstico não bloqueiem a thread de simulação.

#### AC-018 — O produtor nunca executa I/O do sink

- **Dado** um sink lento e uma fila SPSC de capacidade fixa
- **Quando** o produtor publica amostras enquanto o consumidor grava em outra thread
- **Então** o produtor executa somente a operação limitada da fila, não chama callbacks de I/O e perdas por saturação são contabilizadas sem espera por espaço

#### AC-019 — O encerramento drena todas as amostras aceitas

- **Dado** produção concluída entre uma consulta vazia e a próxima iteração do consumidor
- **Quando** o logger recebe a indicação de fim, Stop ou Error recuperável
- **Então** ele encerra somente depois de consumir todas as amostras já aceitas, tenta flush e close e informa quantidades aceitas, persistidas, descartadas e a última sequência persistida

#### AC-020 — Falhas do sink são visíveis e não encerram a simulação

- **Dado** falha injetada em abertura, escrita de cabeçalho, escrita de registro, flush ou close
- **Quando** o logger processa a execução
- **Então** o resultado identifica a primeira etapa e o erro, marca o registro incompleto e permite que o produtor continue até seu encerramento sem prints no caminho crítico

#### AC-021 — A fila mantém ordem sob produtor e consumidor concorrentes

- **Dado** produtor e consumidor reais com wrap e volume superior à capacidade física da fila ao longo do teste
- **Quando** ambos executam concorrentemente sem saturação intencional
- **Então** toda amostra aceita é consumida uma vez, em ordem, e o contador de descarte permanece zero

### US-009 — Converter o registro para CSV após a execução

Como usuário do MICROHIL, quero exportar o registro fechado para CSV usando os metadados da FMU selecionada, para analisar os outputs sem executar a FMU durante a conversão.

#### AC-022 — Conversão durante Running é recusada

- **Dado** um registro ainda aberto pela execução
- **Quando** a conversão é solicitada
- **Então** nenhuma saída CSV é criada ou substituída e o resultado informa que a execução precisa estar encerrada

#### AC-023 — Identidade e esquema incompatíveis impedem a conversão

- **Dado** hash de FMU, índice XML, nome, tipo, valueReference ou ordem divergente do cabeçalho
- **Quando** o conversor compara o registro com o descritor extraído da FMU selecionada
- **Então** a conversão é recusada com a primeira divergência identificada, mesmo quando a quantidade de saídas coincide

#### AC-024 — O CSV preserva tipos, qualidade e registros completos

- **Dado** um registro fechado compatível contendo todos os tipos, valores inválidos, lacunas de sequência e uma cauda opcional truncada
- **Quando** a conversão termina
- **Então** o CSV contém somente registros binários completos na ordem gravada, representa Real inválido como NaN e discretos inválidos como zero, informa qualidade/lacunas/truncamento e nunca apresenta a exportação parcial como integral

### US-010 — Relatar o resultado do logging separadamente da simulação

Como operador, quero receber o resultado agregado do logger ao fim da execução, para distinguir sucesso da simulação, perda de amostras e falha de persistência sem depender de saída no terminal.

#### AC-025 — O resultado agregado não altera as métricas da simulação

- **Dado** a mesma sequência produzida com logging habilitado, desabilitado ou com falha injetada
- **Quando** os resultados finais são agregados
- **Então** o estado e os contadores próprios do logger são apresentados separadamente, campos não medidos não viram zero medido e os agregados temporais da simulação permanecem sob responsabilidade da TASK-004

#### AC-026 — Diagnóstico próprio usa retorno estruturado

- **Dado** debug desligado e uma falha do logger
- **Quando** o consumidor registra o diagnóstico
- **Então** a causa fica disponível por código, etapa e mensagem limitada para GUI/terminal debug, sem `printf`, `fprintf` ou `perror` no novo módulo de logging

## Estado da evidência

Os 12 critérios AC-015…AC-026 foram cobertos por testes C anotados e exercitados no HOST. Isto prova o comportamento controlado do codec, logger, conversor e agregação, não uma execução de FMU ou validação de produto. A fila de capacidade 128, o caminho de terminal com I/O textual legado e o agendamento atual permanecem limitações explicitamente registradas na TASK-002 e na evidência.

## Fora de escopo

- Implementar GUI Qt, seleção visual, atuação DAQC, USB, ROS 2/micro-ROS ou firmware.
- Alterar a grade temporal, medir deadlines ou concluir os agregados da TASK-004.
- Executar uma FMU como prova funcional; os testes usam descritores e amostras controladas.
- Migrar globalmente o código legado; arquivos novos seguem a constituição e módulos tocados preservam APIs de terceiros.
- Definir persistência de métricas por passo, CRC, replay ou recuperação de amostras descartadas.

## Suposições

Nenhuma. Layout, política de invalidade, vínculo com a FMU, conversão posterior e continuidade diante de falha estão definidos no IF-LOG e nos requisitos aprovados.

## Perguntas em aberto

Nenhuma. Políticas de GUI e apresentação visual permanecem nas tarefas correspondentes e não bloqueiam as APIs HOST desta feature.
