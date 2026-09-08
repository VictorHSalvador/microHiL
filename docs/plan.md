# Plano e backlog de retomada

Revisão 0.7. Decisões de produto e ICD foram consolidados; concluir a verificação cruzada dos Markdown e, em seguida, iniciar implementação incremental aproveitando as fontes existentes. Parâmetros dependentes de medição permanecem critérios de cada tarefa e não impedem os componentes HOST independentes. Datas de entrega e duração de sprint não foram definidas.

## TASK-000 — Preparar documentação

Escopo documental: preservar fontes, revisar os 53 IDs de origem e seis derivados (59), incorporar respostas DEC-001…012 e Q-01…09, sincronizar requisitos, alvo, arquitetura, contratos, testes e rastreabilidade e manter a baseline em [sdd-versions.md](sdd-versions.md). Não declarar implementação ou validação do produto por resultado documental.

## Ordem e critérios

| Tarefa | Entrega e requisitos | Dependências | Verificação/critério de pronto | Estado |
|---|---|---|---|---|
| TASK-001 | Build reproduzível e harness HOST; NF-01/11/13/18 | Versão/origem FMILibrary a identificar; sem hardware | Build limpo e versões registradas, testes HOST descobertos; falha de integração declarada se dependência faltar | Preparada em [ficha](tasks/TASK-001.md); não executada |
| TASK-002 | Sink binário, fila, CSV posterior e resultado; F-07/09/11/12/22/25/26, NF-30 | Contrato de erro independente do formato; DEC-008 para política operacional | Falha de log mantém run com aviso de incompletude; erros abertura/escrita/close visíveis; última amostra preservada sob interleaving controlado; ordem e contador sob concorrência | Planejada; correções HOST independentes |
| TASK-003 | Validação, FMU, retenção de inputs DAQC no host, proteção por passo e estados; F-01/05/06/07/08/09/21/23/24/25, NF-14 | TASK-001 para integração; DEC-011 para fixtures/suporte | Inputs inválidos rejeitados antes de threads, initialize/run/stop/restart/error e outputs iniciais/zero de encerramento testados, sem sucesso de zero passos por NaN | Planejada |
| TASK-004 | Ciclo/métricas/recursos; F-12/13, NF-03/07/08/28/29/30/31/32 | 100 Hz como referência e timeout até 5 ms; grade fixa confirmada; correlação/aceitação no ICD | Relógio controlado cobre wakeup tardio/overrun/stop; médias/máximos consistentes; resultados de carga identificam ambiente | Planejada; parâmetros finais pendentes |
| TASK-005 | Migração de estilo e revisão de comentários; NF-26/27 | ADR-001 já confirmada; build/testes disponíveis | Código próprio revisado, declarações/chamadas consistentes, sem renomear terceiros, sem alteração funcional ou quebras cosméticas | Planejada; pode acompanhar módulos já testados |
| TASK-006 | Perfil abstrato, inputs virtuais e DAQC mock; F-02/03/17/21/23/24/27, NF-14/16/19/31 | Contrato interno e TASK-003; perfil sintético rotulado | Mapa tipado rejeita incompatibilidades; snapshot consistente; mock injeta idade/falha sem ser confundido com bancada | Planejada |
| TASK-007 | Implementação host de comunicação; F-18/19/20/23/24/27, NF-02/04/05/06/07/08/18/19/20/21/22/23/24/25/29 | ICD 0.7 e DEC-001/004/006/012 | Codec/parser, frames fragmentados/agregados, gap sem retransmissão, mailboxes, READ_ACK, multiplexação XRCE e testes de integração | Pronta após G-CONSOLIDACAO |
| TASK-008 | Firmware DAQC/perfil físico; F-02/03/18/19/20/27, NF-16/17/19 | TARGET, TASK-007 e toolchain/placa disponíveis | Boot, ADC/DAC/digital/PWM, dois núcleos, parser sem CRC, perda sem replay e DISABLE após 60 s sem READ_ACK; bancada com instrumentos | Perfil lógico pronto; validação física depende do hardware |
| TASK-009 | GUI Qt e persistência; F-04/05/06/07/08/09/10/14/15/16/17/24/25/26, NF-09/10/12/15 | TASK-003/006; Qt/estilo/CSV posterior confirmados; Q-07 para layout do registro | Play/Stop/entradas ao vivo, gráficos individuais até 10 Hz, ticks Y e destruição/reabertura, leitura/escrita/corrupção de perfil/log, falha gráfica isolada | Planejada; detalhes pendentes |
| TASK-010 | Integração HIL e qualificação; conjunto aplicável | TASK-004/007/008 e alvo instrumentado | Malha física e falhas sob carga, limites aprovados, evidências por requisito, nenhum resultado fictício promovido | Não iniciada |

TASK-002 tem partes independentes de FMILibrary/hardware e pode iniciar após a verificação documental. Tarefas do transporte seguem o contrato 0.7; valores de baud, MTU, filas e cadência são escolhidos por medição dentro das tarefas, sem inventar resultados.

## Estratégia de reaproveitamento

- Preservar wrapper, fila, thread periódica e consumidores; após liberação documental, corrigir defeitos por testes antes de ampliar escopo. O CSV existente inspira o conversor pós-run, não continua como logging ao vivo de produto.
- Manter CLI debug e gnuplot como ferramentas transitórias até a GUI cumprir os requisitos; não declarar substituição normativa de Qt.
- Não remover build versionado ou reestruturar pastas de produção sem tarefa específica e verificação. O primeiro build novo deve usar diretório separado.
- Migrar funções/types de C somente com declarações/chamadores e testes correspondentes; revisão de inglês em comentários faz parte da migração.
- A FMU atual continua fixture de outputs; obter/adicionar fixture de inputs e export apropriado para o alvo, sem modificar sua origem para fingir suporte ARM.
- Quando consumidores e controlador forem separados, criar bibliotecas/targets conforme necessidade real; não gerar arquivos vazios para cada módulo proposto.

## Reuso da referência externa

RaspDAQ: adaptar ownership do serviço, snapshots sob mutex curto e coordenação USB/ROS. Não copiar FunctionFS, códigos de protocolo, limpeza de saídas em Stop ou prints do serviço: hardware, ICD, filtro de inputs no host e debug MICROHIL diferem. Escolha Python permanece proposta; thread C crítica não deve herdar alocação de dataclasses por ciclo.

## Conclusão de tarefa

Registrar requisito/contrato, alteração, ambiente, comando, resultado observado, limitações e evidência. Atualizar [matriz](traceability.md). Um teste planejado continua sem evidência até execução. Não marcar tarefa de hardware concluída por mocks. Backlog organiza execução; não substitui a especificação nem precisa de um documento adicional de sprint nesta etapa.

Consolidação 0.7: reusar CONFIG/DATA/schema/ownership do RaspDAQ. DATA não usa CRC, sessão ou retransmissão; gap segue para o pacote novo. TASK-007/008 acrescentam READ_ACK MID 03 e XRCE MID 04, sem copiar o CLI de Sandbox como driver CH340. Arquivos binários continuam design próprio IF-LOG.
