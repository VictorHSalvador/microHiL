# Plano e backlog de retomada

Revisão 0.2. Primeiro concluir esclarecimentos Q-01…09 e consolidar Markdown; somente então iniciar implementação, inclusive correções HOST, conforme instrução explícita do usuário. Plano de execução posterior, aproveitando fontes existentes. O pacote documental pode estar completo para um incremento HOST sem estar liberado para atuação física. Datas de entrega e duração de sprint não foram definidas.

## TASK-000 — Preparar documentação

Escopo documental: preservar fontes, revisar os 53 IDs de origem e cinco derivados (58), incorporar respostas DEC-001…012, manter dúvidas Q explícitas e sincronizar requisitos, alvo, arquitetura, contratos, testes e rastreabilidade. Saída: documentos revisáveis no principal. Não declarar requisitos de produto aprovados nem refatoração concluída por esse resultado.

## Ordem e critérios

| Tarefa | Entrega e requisitos | Dependências | Verificação/critério de pronto | Estado |
|---|---|---|---|---|
| TASK-001 | Build reproduzível e harness HOST; NF-01/11/13/18 | Versão/origem FMILibrary a identificar; sem hardware | Build limpo e versões registradas, testes HOST descobertos; falha de integração declarada se dependência faltar | Preparada em [ficha](tasks/TASK-001.md); não executada |
| TASK-002 | Sink binário, fila, CSV posterior e resultado; F-07/09/11/12/22/25/26, NF-30 | Contrato de erro independente do formato; DEC-008 para política operacional | Erros abertura/escrita/close visíveis; última amostra preservada sob interleaving controlado; ordem e contador sob concorrência | Planejada; correções HOST independentes |
| TASK-003 | Validação, FMU, retenção, proteção e estados; F-01/05/06/07/08/09/21/23/24/25, NF-14 | TASK-001 para integração; DEC-011 para fixtures/suporte | Inputs inválidos rejeitados antes de threads, initialize/run/stop/restart/error testados, sem sucesso de zero passos por NaN | Planejada |
| TASK-004 | Ciclo/métricas/recursos; F-12/13, NF-03/07/08/28/29/30/31/32 | 100 Hz como referência e timeout até 5 ms; Q-06 para escopo/agenda/aceitação | Relógio controlado cobre wakeup tardio/overrun/stop; médias/máximos consistentes; resultados de carga identificam ambiente | Planejada; parâmetros finais pendentes |
| TASK-005 | Migração de estilo e revisão de comentários; NF-26/27 | ADR-001 já confirmada; build/testes disponíveis | Código próprio revisado, declarações/chamadas consistentes, sem renomear terceiros, sem alteração funcional ou quebras cosméticas | Planejada; pode acompanhar módulos já testados |
| TASK-006 | Perfil abstrato, inputs virtuais e DAQC mock; F-02/03/17/21/23, NF-14/16/19/31 | Contrato interno e TASK-003; perfil sintético rotulado | Mapa tipado rejeita incompatibilidades; snapshot consistente; mock injeta idade/falha sem ser confundido com bancada | Planejada |
| TASK-007 | Decisão e implementação host de comunicação; F-18/19/20, NF-02/04/05/06/07/08/18/19/20/21/22/23/24/25/29 | DEC-001/004/006/012 resolvidas e ICD liberado | Serialização/parser ou mensagens conforme decisão, frames fragmentados e recuperação; versões e testes de integração | Bloqueada apenas no transporte definitivo |
| TASK-008 | Firmware DAQC/perfil físico; F-02/03/18/19/20/23, NF-16/17/19 | TARGET e DEC-003/004/005/006, TASK-007 | Boot, estado seguro, ADC/DAC/digital, watchdog, reset, timeout e reconexão em bancada com instrumentos | Bloqueada no hardware/contrato |
| TASK-009 | GUI Qt e persistência; F-04/05/06/07/08/09/10/14/15/16/17/24/25/26, NF-09/10/12/15 | TASK-003/006; Qt/estilo/CSV posterior confirmados; Q-03/Q-07 para detalhes | Play/Stop/entradas ao vivo, gráficos individuais até 10 Hz, ticks Y e destruição/reabertura, leitura/escrita/corrupção de perfil/log, falha gráfica isolada | Planejada; detalhes pendentes |
| TASK-010 | Integração HIL e qualificação; conjunto aplicável | TASK-004/007/008 e alvo instrumentado | Malha física e falhas sob carga, limites aprovados, evidências por requisito, nenhum resultado fictício promovido | Não iniciada |

Tecnicamente TASK-002 tem partes independentes de FMILibrary/hardware, mas não serão implementadas/testadas nesta fase: o usuário pediu concluir os Markdown primeiro. A sequência posterior continua aproveitando essa independência; nenhuma pendência é respondida por suposição.

## Estratégia de reaproveitamento

- Preservar wrapper, fila, thread periódica e consumidores; após liberação documental, corrigir defeitos por testes antes de ampliar escopo. O CSV existente inspira o conversor pós-run, não continua como logging ao vivo de produto.
- Manter CLI debug e gnuplot como ferramentas transitórias até a GUI cumprir os requisitos; não declarar substituição normativa de Qt.
- Não remover build versionado ou reestruturar pastas de produção sem tarefa específica e verificação. O primeiro build novo deve usar diretório separado.
- Migrar funções/types de C somente com declarações/chamadores e testes correspondentes; revisão de inglês em comentários faz parte da migração.
- A FMU atual continua fixture de outputs; obter/adicionar fixture de inputs e export apropriado para o alvo, sem modificar sua origem para fingir suporte ARM.
- Quando consumidores e controlador forem separados, criar bibliotecas/targets conforme necessidade real; não gerar arquivos vazios para cada módulo proposto.

## Conclusão de tarefa

Registrar requisito/contrato, alteração, ambiente, comando, resultado observado, limitações e evidência. Atualizar [matriz](traceability.md). Um teste planejado continua sem evidência até execução. Não marcar tarefa de hardware concluída por mocks. Backlog organiza execução; não substitui a especificação nem precisa de um documento adicional de sprint nesta etapa.
