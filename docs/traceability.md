# Matriz de rastreabilidade

Revisão 0.7. [spec.md](spec.md) preserva 53 IDs de origem e acrescenta F-23…27/NF-32 (59 requisitos). Decisões confirmadas não alteram o estado de implementação: fontes continuam no estado auditado em `c788a4283cf81f17e9a2956ae258487c7931590a`.

I = mecanismo identificado, não aceitação integral; P = parcial; A = ausente; D = divergente; NA = sem componente. Design em [architecture.md](architecture.md), contratos em [ICD](contracts/interfaces.md), tarefas em [plan.md](plan.md). Todos os V-* permanecem planejados. Evidências históricas não aprovam as exigências revisadas nem foram executadas novamente.

| Requisito | Design/contrato | Tarefa | Código observado | Estado | Teste planejado / ambiente | Evidência disponível |
|---|---|---|---|---|---|---|
| REQ-F-01 | ARCH-CORE / IF-CORE | TASK-003 | src/fmu_model.c; src/main.c; src/app_config.c | I | V-F-01 / HOST | [EV-AUD-03](evidence/audit-2026-09-06/README.md): metadados; sem run |
| REQ-F-02 | ARCH-IO / IF-CORE | TASK-006 | Não implementado | A | V-F-02 / HOST + bancada/HIL | Sem execução do caso de aceitação |
| REQ-F-03 | ARCH-IO / IF-DAQ | TASK-006, TASK-008 | Não implementado | A | V-F-03 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-F-04 | ARCH-GUI / IF-LOG | TASK-009 | Não implementado | A | V-F-04 / HOST + GUI | Sem execução do caso de aceitação |
| REQ-F-05 | ARCH-CORE / IF-CORE | TASK-003, TASK-009 | src/fmu_model.c; src/main.c; src/app_config.c | P | V-F-05 / HOST + GUI | Sem execução do caso de aceitação |
| REQ-F-06 | ARCH-STATE / IF-CORE | TASK-003, TASK-009 | src/main.c; src/rt_simulation.c (sem FSM) | P | V-F-06 / HOST + GUI | Sem execução do caso de aceitação |
| REQ-F-07 | ARCH-STATE / IF-LOG | TASK-002, TASK-003, TASK-009 | src/main.c; src/rt_simulation.c (sem FSM) | P | V-F-07 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-F-08 | ARCH-STATE | TASK-003, TASK-009 | Não implementado | A | V-F-08 / HOST + GUI | Sem execução do caso de aceitação |
| REQ-F-09 | ARCH-STATE / IF-LOG | TASK-002, TASK-003, TASK-009 | src/main.c; src/rt_simulation.c (sem FSM) | P | V-F-09 / HOST + HIL | [EV-AUD-02](evidence/audit-2026-09-06/README.md): logger reprovado; fila só sequencial |
| REQ-F-10 | ARCH-GUI / IF-LOG | TASK-009 | src/plotter.c; src/main.c (sem Qt) | D | V-F-10 / HOST + GUI | Sem execução do caso de aceitação |
| REQ-F-11 | ARCH-LOG / IF-LOG | TASK-002 | src/csv_logger.c; src/sample_queue.c | P | V-F-11 / HOST + HIL | [EV-AUD-02](evidence/audit-2026-09-06/README.md): logger reprovado; fila só sequencial |
| REQ-F-12 | ARCH-TIME / IF-LOG | TASK-002, TASK-004 | src/rt_simulation.c; agregados incompletos | P | V-F-12 / HOST + alvo | Inspeção apenas; sem caso de aceitação executado |
| REQ-F-13 | ARCH-TIME | TASK-004 | src/rt_simulation.c; include/rt_simulation.h | P | V-F-13 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-F-14 | ARCH-GUI | TASK-009 | src/plotter.c; src/main.c (sem Qt) | P | V-F-14 / GUI | Sem execução do caso de aceitação |
| REQ-F-15 | ARCH-GUI | TASK-009 | src/plotter.c; src/main.c (sem Qt) | P | V-F-15 / GUI | Sem execução do caso de aceitação |
| REQ-F-16 | ARCH-GUI | TASK-009 | Não implementado | A | V-F-16 / GUI + alvo | Sem execução do caso de aceitação |
| REQ-F-17 | ARCH-CORE / IF-CORE | TASK-006, TASK-009 | Não implementado | A | V-F-17 / HOST + GUI | Sem execução do caso de aceitação |
| REQ-F-18 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-F-18 / HOST + bancada | Sem execução do caso de aceitação |
| REQ-F-19 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-F-19 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-F-20 | ARCH-IO / IF-DAQ | TASK-007, TASK-008 | Não implementado | A | V-F-20 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-F-21 | ARCH-CORE / IF-CORE | TASK-003, TASK-006 | src/fmu_model.c; src/main.c; src/app_config.c | P | V-F-21 / HOST | Sem execução do caso de aceitação |
| REQ-F-22 | ARCH-LOG / IF-LOG | TASK-002 | src/csv_logger.c; src/sample_queue.c | P | V-F-22 / HOST | [EV-AUD-02](evidence/audit-2026-09-06/README.md): logger reprovado; fila só sequencial |
| REQ-F-23 | ARCH-CORE / ARCH-IO / IF-SAMPLE | TASK-003, TASK-006, TASK-007 | Não implementado para inputs DAQC | A | V-F-23 / HOST + integração | Sem execução da revisão 0.4 |
| REQ-F-24 | ARCH-CORE / ARCH-STATE / IF-SAMPLE | TASK-003, TASK-006, TASK-007, TASK-009 | Não implementado para inputs DAQC | A | V-F-24 / HOST + integração | Sem execução da revisão 0.4 |
| REQ-F-25 | ARCH-STATE / ARCH-LOG | TASK-002, TASK-003, TASK-009 | src/main.c e wrapper: terminal sem gate debug | P | V-F-25 / HOST + GUI | Sem execução |
| REQ-F-26 | ARCH-LOG / IF-LOG | TASK-002, TASK-009 | Não implementado | A | V-F-26 / HOST + GUI | Sem execução |
| REQ-F-27 | ARCH-FW / IF-READ-PROGRESS | TASK-006, TASK-007, TASK-008, TASK-010 | Não implementado | A | V-F-27 / HOST + bancada/HIL | Sem execução; READ_ACK definido, cadência/tolerância dependem de medição |
| REQ-NF-01 | ARCH-CORE | TASK-001 | CMakeLists.txt; src/; include/ | P | V-NF-01 / HOST + alvo | [EV-AUD-01](evidence/audit-2026-09-06/README.md): configuração integral bloqueada |
| REQ-NF-02 | ARCH-IO | TASK-007 | Não implementado | A | V-NF-02 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-NF-03 | ARCH-TIME | TASK-004 | src/rt_simulation.c; include/rt_simulation.h | P | V-NF-03 / HOST + alvo | Sem execução do caso de aceitação |
| REQ-NF-04 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-NF-04 / HOST + alvo | Sem execução do caso de aceitação |
| REQ-NF-05 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-NF-05 / Bancada | Sem execução do caso de aceitação |
| REQ-NF-06 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-NF-06 / HOST + bancada | Sem execução do caso de aceitação |
| REQ-NF-07 | ARCH-TIME / IF-DAQ | TASK-004, TASK-007 | Não implementado | A | V-NF-07 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-NF-08 | ARCH-IO / IF-SAMPLE | TASK-004, TASK-007 | Não implementado | A | V-NF-08 / HOST + alvo | Sem execução do caso de aceitação |
| REQ-NF-09 | ARCH-GUI | TASK-009 | src/plotter.c; src/main.c (sem Qt) | D | V-NF-09 / GUI + alvo | Sem execução do caso de aceitação |
| REQ-NF-10 | ARCH-GUI | TASK-009 | src/plotter.c; src/main.c (sem Qt) | P | V-NF-10 / HOST + alvo | Sem execução do caso de aceitação |
| REQ-NF-11 | ARCH-CORE | TASK-001 | CMakeLists.txt; src/; include/ | I | V-NF-11 / HOST | Inspeção das fontes C; build integral pendente |
| REQ-NF-12 | ARCH-GUI | TASK-009 | src/plotter.c; src/main.c (sem Qt) | D | V-NF-12 / HOST + GUI | Sem execução do caso de aceitação |
| REQ-NF-13 | ARCH-CORE | TASK-001 | CMakeLists.txt; src/; include/ | I | V-NF-13 / HOST | [EV-AUD-01](evidence/audit-2026-09-06/README.md): configuração integral bloqueada |
| REQ-NF-14 | ARCH-CORE / IF-CORE | TASK-003, TASK-006 | src/fmu_model.c; src/main.c; src/app_config.c | P | V-NF-14 / HOST | Sem execução do caso de aceitação |
| REQ-NF-15 | ARCH-CORE / ARCH-IO / ARCH-GUI | TASK-003, TASK-007, TASK-009 | src/fmu_model.c; src/main.c; src/app_config.c | P | V-NF-15 / HOST | Sem execução do caso de aceitação |
| REQ-NF-16 | ARCH-IO / IF-CORE | TASK-006, TASK-008 | Não implementado | A | V-NF-16 / HOST + bancada | Sem execução do caso de aceitação |
| REQ-NF-17 | ARCH-FW / IF-DAQ | TASK-008 | Não implementado | A | V-NF-17 / Bancada + HIL | Sem execução do caso de aceitação |
| REQ-NF-18 | ARCH-IO | TASK-001, TASK-007 | Não implementado | A | V-NF-18 / HOST | Sem execução do caso de aceitação |
| REQ-NF-19 | ARCH-IO / IF-DAQ | TASK-006, TASK-007, TASK-008 | Não implementado | A | V-NF-19 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-NF-20 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-NF-20 / HOST + bancada | Sem execução do caso de aceitação |
| REQ-NF-21 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-NF-21 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-NF-22 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-NF-22 / HOST + bancada | Sem execução do caso de aceitação |
| REQ-NF-23 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-NF-23 / HOST + bancada | Sem execução do caso de aceitação |
| REQ-NF-24 | ARCH-IO / IF-DAQ | TASK-007, TASK-008 | Não implementado | A | V-NF-24 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-NF-25 | ARCH-IO / IF-DAQ | TASK-007 | Não implementado | A | V-NF-25 / HOST + bancada | Sem execução do caso de aceitação |
| REQ-NF-26 | ADR-001 / constitution | TASK-005 | src/ e include/ ainda não migrados | D | V-NF-26 / Revisão + HOST | Sem execução do caso de aceitação |
| REQ-NF-27 | ADR-001 / constitution | TASK-005 | Sem Python de produto | NA | V-NF-27 / Revisão + HOST | Sem execução do caso de aceitação |
| REQ-NF-28 | ARCH-TIME | TASK-004 | src/rt_simulation.c; include/rt_simulation.h | P | V-NF-28 / HOST + alvo | Sem execução do caso de aceitação |
| REQ-NF-29 | ARCH-IO / IF-DAQ | TASK-004, TASK-007 | Não implementado | A | V-NF-29 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-NF-30 | ARCH-LOG / ARCH-GUI | TASK-002, TASK-004, TASK-009 | src/csv_logger.c; src/sample_queue.c | P | V-NF-30 / HOST + alvo | Sem execução do caso de aceitação |
| REQ-NF-31 | ARCH-TIME / IF-SAMPLE | TASK-004, TASK-006, TASK-007 | src/rt_simulation.c; include/rt_simulation.h | P | V-NF-31 / HOST + HIL | Sem execução do caso de aceitação |
| REQ-NF-32 | ARCH-TIME | TASK-004, TASK-010 | Não implementado | A | V-NF-32 / HOST + HIL | Sem execução |
