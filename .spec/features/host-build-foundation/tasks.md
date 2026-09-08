# Tasks: Fundação de build HOST

> feature: host-build-foundation

## T-002 — Separar o build independente e integrar a dependência fixa [pendente]

- Refs: US-004, AC-007, AC-008, US-005, AC-010, AC-011, AC-012
- Arquivos: CMakeLists.txt, cmake/fmilib.cmake
- Notas: preservar o runner existente; `MICROHIL_BUILD_RUNNER=OFF` não procura FMILibrary; obtenção automática usa a release oficial 3.0.4 em revisão fixa; instalação externa permanece opção explícita.

## T-003 — Criar o harness CTest e os testes da fila [pendente]

- Refs: US-004, AC-007, AC-008, AC-009
- Arquivos: CMakeLists.txt, tests/CMakeLists.txt, tests/test_sample_queue.c, test/run_spec_tests.js, onpspec.config.json
- Notas: depende de T-002; testar ordem, wrap e saturação sem FMILibrary e sem transformar teste sequencial em prova de concorrência ainda não executada.

## T-004 — Documentar, evidenciar e rastrear a TASK-001 [pendente]

- Refs: US-005, AC-010, AC-011, AC-012, US-006, AC-013, AC-014
- Arquivos: README.md, docs/README.md, docs/TARGET.md, docs/plan.md, docs/tasks/TASK-001.md, docs/traceability.md, docs/quality-gates.md, docs/sdd-versions.md, docs/evidence/host-build-foundation-2026-09-07.md, .spec/features/host-build-foundation/spec.md, .spec/features/host-build-foundation/tasks.md
- Notas: depende dos resultados observados de T-002 e T-003; atualizar a baseline SDD somente depois de verify e audit aprovados.
