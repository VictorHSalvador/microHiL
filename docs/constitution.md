# Constituição de desenvolvimento

Aplicação: MICROHIL. Revisão de trabalho 1, 06.09.2026. Regras derivadas das instruções do usuário; opções de produto pendentes permanecem em [decisions.md](decisions.md).

## Especificação e evidência

Cada incremento deve identificar requisito, comportamento observável, design, teste e evidência. Não usar compilação como sinônimo de validação de produto. Não renumerar requisitos recebidos para imitar o Demo. Especificações de firmware só podem fixar valores físicos com fonte, decisão e perfil correspondentes.

Resultados devem separar esperado de observado e informar ambiente, procedimento, versão e limitações. Falhas permanecem registradas. Um máximo medido não prova WCET universal. Dados fictícios ou do Demo nunca aprovam o MICROHIL.

## Código e comentários

Todo código próprio de produção deve usar comentários de desenvolvedor, breves e úteis, em português ou inglês. Comentários explicam motivo, invariante, ownership, restrição, unidade ou contrato não óbvio; não narram cada linha, não ensinam sintaxe básica, não mencionam IA e não funcionam como ornamento. Comentários novos e revisados não justificam quebra cosmética de chamadas ou assinaturas. Preservar o conteúdo útil de comentários existentes; traduzir ou ajustar somente no incremento revisado. Documentação funcional e explicações ao usuário permanecem em português. Comentários do código de terceiros e snapshots de evidência conservam a origem.

| Elemento | C | Python |
|---|---|---|
| Funções | PascalCase: `ReadOutputs()` | snake_case: `read_outputs()` |
| Métodos | Não aplicável | snake_case |
| Variáveis globais/locais e campos próprios | snake_case: `cycle_count` | snake_case |
| Constantes e macros | UPPER_CASE: `MAX_OUTPUTS` | UPPER_CASE |
| Tipos customizados/typedef/struct | snake_case com `_t`: `simulation_config_t` | Classes PascalCase: `SimulationConfig` |
| Arquivos | snake_case: `fmu_model.c` | Minúsculas curtas; snake_case se melhorar leitura |
| Pacotes | Conforme organização de build | Mesma regra dos módulos |

Tags próprias de enum/struct e typedefs seguem a regra de tipos; enumeradores seguem UPPER_CASE. APIs, tipos e macros de terceiros não são renomeados. `main` e outros pontos de entrada obrigatórios mantêm ABI. Exemplos usam identificadores em inglês para continuidade com o código existente; a instrução do usuário determina a forma dos nomes e o idioma dos comentários, não exige renomear APIs externas.

Evitar quebras artificiais em listas de parâmetros, argumentos e expressões. Assinaturas e chamadas permanecem em uma linha quando legíveis. Quebrar somente se a linha ficar extremamente longa; não foi aprovado um limite numérico. PEP 8 rege nomenclatura Python, com a exceção explícita de que não será imposta quebra cosmética por largura de linha. A formatação não deve juntar blocos lógicos inteiros em uma linha. Não executar formatador com defaults incompatíveis.

Convenção de design para Qt/C++ (DEC-010): classes e métodos próprios PascalCase, variáveis e membros snake_case, constantes UPPER_CASE e arquivos snake_case; classes C++ não recebem `_t`. Overrides, signals/slots exigidos por bibliotecas e todas as APIs Qt conservam os nomes de origem. Comentários em inglês e regra de quebra já se aplicam. A interface exportada pelo núcleo C obedece ao estilo C. A migração dos símbolos existentes será incremental, com atualização de declarações, chamadas e testes no mesmo incremento.

## Concorrência e recursos

Cada dado compartilhado deve ter escritor/proprietário e mecanismo de acesso definidos. Definir limites de filas e política de saturação por consumidor. Não realizar logging textual, operações de disco, callbacks GUI/ROS ou espera não limitada no caminho crítico da solução projetada. Prints de código próprio só são permitidos com debug habilitado e devem ocorrer em consumidor não crítico; debug não autoriza prints no ciclo. Diagnósticos para GUI permanecem disponíveis sem terminal. Mensagens/callbacks de FMU devem ser encaminhados por mecanismo limitado; prints internos de binários de terceiros precisam de avaliação, não de uma promessa de supressão que o host não consegue garantir. O código legado ainda possui exceções registradas na avaliação, que devem ser corrigidas, não negadas documentalmente.

Inicialização, buffers e alocações devem ficar fora da fase periódica quando controlados pelo projeto. Memória/stack e alocações de FMUs e middleware precisam de avaliação no ambiente efetivo; não aplicar automaticamente o limite de 256 KiB do Demo ao processo Linux. Políticas, prioridade, jitter, deadline e overrun devem ser explícitos antes da aceitação temporal.

## Hardware e falhas

Não inventar placa, pinos, faixas, circuitos, clocks, taxa de comunicação ou estado elétrico. F-23/F-24 tratam aquisição mundo real → DAQC → USB → input FMU no host: reter último válido e, sem histórico, valor inicial válido do input. Com checkbox ativo, 100 passos inválidos consecutivos por canal encerram em Error. F-27 exige DISABLE após 60 s de streaming sem progresso de leitura confirmado pelo host, sem zeramento por amostra inválida isolada; término da execução exige zero físico. Não aplicar essas políticas automaticamente a outputs FMU/atuadores. Usar os dois núcleos ESP32, com supervisão fora da tarefa crítica, sem bloqueios ilimitados e com interferência medida.

Não iniciar atuação física com perfil incompleto. Boot, reset, comunicação perdida, watchdog e recuperação devem ter critérios de bancada. Ausência de esquemático não impede trabalho HOST, modelagem ou testes simulados; limita os ensaios e drivers que dependem dele.

## Gestão de mudanças

Registrar decisões confirmadas e impacto nos IDs. Propostas podem orientar análise e mocks, sem se transformar em contrato físico aprovado. Priorizar correções funcionais e reuso antes de refatorações extensas. O repositório deve ser a referência durável; conversas adicionam decisões que precisam ser refletidas aqui.

Revisão 0.5: leitor USB em thread independente; consumir última atualização antes da inserção FMI; ausência conta timeout separado. Encerrar simulação zera AO/DO/PWM e encerra DATA, enquanto novo Play reaplica outputs iniciais da FMU reinicializada. Aguardar próximo instante da grade fixa após overrun, sem compensar nem alterar passos do modelo; relatório temporal somente no fim. ADC/PWM configuráveis dentro das opções explícitas do TARGET.

Revisão 0.7: comunicação DATA é best effort, sem CRC ou recuperação de pacote. Não adicionar retransmissão, replay ou fila crescente ao caminho de tempo real. CONFIG/READ_ACK/XRCE permanecem separados e não podem bloquear a simulação.
