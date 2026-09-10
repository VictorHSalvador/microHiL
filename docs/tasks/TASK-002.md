# TASK-002 Logging binário e conversão posterior no HOST

Status: concluída, evidenciada e auditada mecanicamente em 09.09.2026. Requisitos: REQ-F-07, REQ-F-09, REQ-F-11, REQ-F-12, REQ-F-22, REQ-F-25, REQ-F-26 e REQ-NF-30; REQ-NF-26 é parcial para os módulos novos. Contrato: [IF-LOG](../contracts/interfaces.md#IF-LOG--Registro-binário-e-configuração-contrato-HOST-não-código-RaspDAQ). Gates: G-DOC-HOST e G-HOST, somente nos itens demonstrados.

## Escopo

Substituir o CSV concorrente pelo logging binário `MHILLOG1` no HOST, sem alterar requisitos de produto. O logger registra outputs finais tipados por passo em fila SPSC limitada e um conversor CSV opera somente depois do encerramento. Nenhuma FMU foi executada nesta tarefa; GUI, Raspberry Pi, USB, ROS, firmware, bancada, HIL e timing real também não foram validados.

## Implementação observada

- `log_format` implementa serialização campo a campo little-endian, limites e diagnóstico de cabeçalho/registro; o descritor contém SHA-256 dos bytes da FMU, `t_start`, `h` e saídas Real, Integer, Enumeration e Boolean.
- O índice XML é one-based, ou seja, posição da `ScalarVariable` na lista XML. A ordem canônica é crescente por esse índice; valueReference não é chave única.
- `binary_logger` usa fila SPSC de capacidade fixa 128. O produtor só publica cópia limitada; o consumidor é dono do sink e drena amostras aceitas antes de flush/close. Perdas e a primeira falha ficam no resultado estruturado.
- A capacidade 128 é decisão de implementação pendente de medição; não é dimensionamento aprovado de produto.
- `run_logging` constrói descritor e SHA-256 por implementação C local, sem comando externo em runtime. O runner preserva os tipos FMI, normaliza Real inválido para NaN e discreto inválido para zero com qualidade, e agrega resultados de simulação e logging separadamente.
- `log_converter` exige execução fechada e compara hash, índice XML, nome, tipo, valueReference e ordem com o descritor atual. `t_start` e `h` são formato, não identidade do conversor. CSV usa `sequence`, `simulation_time_s` e pares `<nome>_value`/`<nome>_valid`.
- Cauda truncada exporta apenas registros completos com resultado parcial. Log incompleto por falha de logging não é disponibilizado como log íntegro para a ação de conversão.

## Decisões registradas

Não surgiu decisão arquitetural de produto nova. Esta tarefa materializa no HOST o IF-LOG já aprovado e registra: capacidade 128 pendente de medição; índice XML one-based; identidade do conversor sem `t_start`/`h`; colunas CSV; SHA-256 C local; e bloqueio de conversão como íntegra para log incompleto.

## Verificações e resultado

A [evidência executada](../evidence/host-run-logging-2026-09-08.md) registra build limpo HOST, CTest normal e ASan+UBSan, harness documental, compilação do runner com instalação externa explícita e confirmação mecânica de AC-015…AC-026. Os gates `verify` e `audit --ci` passaram fora do sandbox restrito, pois o motor precisa iniciar o processo-filho do harness. Os testes exercitam descritores/amostras/sinks controlados e concorrência HOST; não são teste de uma FMU.

## Limitações e próximos passos

- O terminal é interface transitória de debug. `printf`/`fprintf` legados na thread de simulação e no wrapper FMI permanecem; não alegar conformidade final de ausência de I/O textual no caminho crítico.
- O agendamento atual não é evidência da política final de grade fixa. TASK-004 deve corrigir/medir timing e métricas completas.
- TASK-003/TASK-009 continuam responsáveis por lifecycle com FMU real, GUI e apresentação ao operador; TASK-005, pela migração de estilo global.
- A compilação do runner não prova execução da FMU nem versão/compatibilidade da instalação externa FMILibrary.
