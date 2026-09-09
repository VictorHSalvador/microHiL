# Tasks: Logging binário e conversão pós-execução no HOST

> feature: host-run-logging

## T-005 — Implementar e testar o codec binário tipado [concluida]

- Refs: US-007, AC-015, AC-016, AC-017
- Arquivos: include/log_format.h, src/log_format.c, tests/test_log_format.c, tests/CMakeLists.txt, CMakeLists.txt
- Notas: depende do IF-LOG; serialização campo a campo little-endian, sem dump de struct; limites verificados antes de alocar; funções novas em PascalCase e tipos próprios snake_case com sufixo `_t`.

## T-006 — Implementar o logger assíncrono e os testes concorrentes [concluida]

- Refs: US-008, AC-018, AC-019, AC-020, AC-021, US-010, AC-026
- Arquivos: include/binary_logger.h, include/sample_queue.h, src/binary_logger.c, src/sample_queue.c, tests/test_binary_logger.c, tests/test_sample_queue.c, tests/CMakeLists.txt, CMakeLists.txt
- Notas: depende de T-005; sink injetável para falhas determinísticas; produtor usa fila limitada e consumidor drena após producer_done; não usar dispositivo cheio como único teste nem imprimir no módulo.

## T-007 — Implementar e testar a conversão pós-execução para CSV [concluida]

- Refs: US-009, AC-022, AC-023, AC-024
- Arquivos: include/log_converter.h, src/log_converter.c, tests/test_log_converter.c, tests/CMakeLists.txt, CMakeLists.txt
- Notas: depende de T-005; comparar hash e todos os campos do esquema; recusar execução aberta; cauda truncada conserva somente registros completos e retorna aviso de parcialidade.

## T-008 — Integrar logging binário ao runner e ao resultado final [concluida]

- Refs: US-008, AC-018, AC-019, AC-020, US-010, AC-025, AC-026
- Arquivos: include/app_config.h, include/common.h, include/fmu_model.h, include/rt_simulation.h, include/run_logging.h, include/run_result.h, include/sha256.h, src/app_config.c, src/fmu_model.c, src/main.c, src/rt_simulation.c, src/run_logging.c, src/run_result.c, src/sha256.c, CMakeLists.txt, tests/test_run_logging.c, tests/CMakeLists.txt, test/run_spec_tests.js
- Notas: depende de T-006 e T-007; substituir CSV concorrente pelo binário, manter plot separado, agregar resultado do logger sem transformar falha de persistência em falha da simulação e conservar conversão como ação posterior.

## T-009 — Atualizar SDD, rastreabilidade e evidência da TASK-002 [concluida]

- Refs: US-007, AC-015, AC-016, AC-017, US-008, AC-018, AC-019, AC-020, AC-021, US-009, AC-022, AC-023, AC-024, US-010, AC-025, AC-026
- Arquivos: README.md, docs/README.md, docs/contracts/interfaces.md, docs/plan.md, docs/tasks/TASK-002.md, docs/traceability.md, docs/quality-gates.md, docs/sdd-versions.md, docs/evidence/host-run-logging-2026-09-08.md, .spec/features/host-run-logging/spec.md, .spec/features/host-run-logging/tasks.md, test/sdd-versioning.spec.test.js, onpspec.config.json
- Notas: depende de T-005 a T-008; registrar somente testes executados, falhas observadas e limites; revisão PATCH se o comportamento permanecer dentro do IF-LOG já aprovado.
