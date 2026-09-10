# Design: Logging binário e conversão pós-execução no HOST

## Fronteiras

O caminho de produção continua: thread de simulação cria uma amostra final por passo e tenta publicá-la em fila SPSC limitada. O novo logger é o único consumidor e o único proprietário do arquivo binário. Ele não chama FMILibrary, GUI, ROS ou DAQC. O conversor opera sobre arquivo fechado e recebe um descritor extraído da FMU selecionada por uma camada externa; não executa a FMU.

## Componentes

| Componente | Responsabilidade | Não faz |
|---|---|---|
| `log_format` | Validar descritor, escrever/ler cabeçalho e registros campo a campo, controlar limites e erros de formato | Threads, UI, acesso à FMU ou política de execução |
| `binary_logger` | Possuir sink e thread consumidora, drenar fila, propagar falhas e produzir resultado agregado | Bloquear produtor, imprimir ou encerrar a simulação |
| `log_converter` | Comparar identidade/esquema e converter registros completos para CSV após fechamento | Converter durante Running, inferir esquema por quantidade ou ocultar truncamento |
| Integração do runner | Criar descritor, iniciar/encerrar logger, publicar amostras e combinar resultados | Escrever disco no ciclo crítico ou converter CSV concorrentemente |

## Contratos internos

O descritor contém SHA-256 da FMU, `t_start`, `h` e até `MAX_OUTPUTS` entradas. Cada entrada contém índice XML, valueReference, tipo e nome. A ordem é validada como índice XML estritamente crescente. Duplicidade de valueReference é permitida. O codec recusa tipo desconhecido, nome vazio/excessivo, contagem excessiva e operações cujo tamanho estoure `size_t` ou os campos do formato.

A amostra de logging contém sequência, tempo simulado, contagem fixa conforme o descritor, valores no tipo interno atual e qualidade por saída. Na serialização, valores discretos precisam caber no tipo do formato; valor incompatível é gravado como zero e marcado inválido. Real não finito é gravado como NaN e marcado inválido. O formato não contém `wall_time_s`, deadline, duração de ciclo ou métricas USB.

O resultado do logger separa abertura, cabeçalho, registro, flush e close. Ele conserva o primeiro erro, conta tentativas aceitas/persistidas/descartadas e identifica se existe última sequência persistida. A thread continua drenando a fila depois de `producer_done`; após falha irreversível do sink, descarta de forma contabilizada as amostras restantes sem tentar I/O repetidamente.

## Concorrência e falhas

Somente o produtor altera `head` e somente o consumidor altera `tail`. Saturação rejeita a amostra nova e incrementa o contador; não sobrescreve a mais antiga. O teste concorrente inicia produtor e consumidor reais, força wrap por várias voltas e verifica ordem/uniquidade. Isso demonstra a execução no HOST ensaiado, não propriedades lock-free universais nem timing no Raspberry Pi.

O sink é uma tabela de callbacks com contexto, permitindo arquivo real em produção e falhas determinísticas nos testes. Nenhum callback do sink é acessível pelo produtor. Mensagens de erro são buffers limitados no resultado e chegam ao coordenador depois do join.

## Conversão

O conversor exige estado de execução fechado fornecido pelo coordenador, abre o binário, valida todo o cabeçalho contra o descritor da FMU e só então cria a saída temporária. Em sucesso, renomeia a saída temporária para o destino; em erro ou incompatibilidade, preserva o destino anterior. Uma cauda truncada termina a leitura com resultado parcial e exporta somente registros completos, incluindo colunas de sequência, tempo simulado, qualidade por saída e valores tipados.

## Limites da feature

Os testes controlam amostras e falhas de sink, sem executar a FMU. SHA-256 identifica os bytes do arquivo FMU, mas não prova compatibilidade de arquitetura ou comportamento. `fsync` e garantias contra perda por queda de energia não foram exigidos; erros retornados por write/flush/close são propagados. A GUI definirá quando e como apresentar o resultado, sem mudar o contrato HOST.
