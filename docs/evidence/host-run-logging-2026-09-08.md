# Evidência HOST — TASK-002 Logging binário e conversão posterior

Data: 08.09.2026. Diretório: `/home/linuxvh/Projects/microHiL`. Escopo: implementação T-005…T-008 e sincronização documental T-009 da feature `host-run-logging`.

## Resultado executado

| Verificação | Comando | Resultado observado |
|---|---|---|
| Build HOST limpo | `task_build_dir=$(mktemp -d /tmp/microhil-host-independent-XXXXXX)`; `cmake -S . -B "$task_build_dir" -DMICROHIL_BUILD_RUNNER=OFF -DBUILD_TESTING=ON`; `cmake --build "$task_build_dir"` | Configurou e compilou os componentes HOST, runner desligado |
| CTest HOST | `ctest --test-dir "$task_build_dir" --output-on-failure` | 23/23 passaram |
| ASan+UBSan | diretório novo por `mktemp`, configuração HOST com `-fsanitize=address,undefined -fno-omit-frame-pointer`; `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir "$task_asan_dir" --output-on-failure` | 23/23 passaram |
| Harness documental | `node test/run_spec_tests.js`; `node --test test/run_spec_tests.js` | Passaram; o agregador emite 29 testes TAP, incluindo AC-015…AC-026 |
| Runner com instalação externa | diretório novo por `mktemp`; CMake com `FMILIB_INCLUDE_DIR=/home/linuxvh/Projects/asturian-software/dev/external/include` e `FMILIB_LIBRARY=/home/linuxvh/Projects/asturian-software/release/external/lib/libfmilib.a` | `fmu_rt_runner` compilou |
| Critérios da feature | busca mecânica de `@spec:AC-015`…`@spec:AC-026` em testes C e harness direto | 12/12 critérios anotados e emitidos pelo harness, que também executa CTest HOST |
| Motor spec-driven | `node --experimental-default-type=module /home/linuxvh/.agents/skills/onp-spec-driven/scripts/onp-spec.mjs verify host-build-foundation`; `verify sdd-versioning`; `verify host-run-logging`; depois `audit --ci` | 8/8, 6/6 e 12/12 critérios com prova PASS; auditoria limpa |

## Cobertura HOST confirmada

Os testes C anotados cobrem AC-015…AC-017 no codec, AC-018…AC-021 e AC-026 no logger, AC-022…AC-024 no conversor e AC-025 na integração/resultado. Cobrem formato little-endian, tipos/qualidade, entradas malformadas, sink lento/falhas, drenagem, wrap/ordem concorrente, conversão posterior, identidade, parcialidade e agregação separada.

## Limites e falhas preservados

- `ASAN_OPTIONS=detect_leaks=0` foi necessário porque LeakSanitizer é incompatível com `ptrace` neste ambiente. O resultado ASan+UBSan não demonstra ausência de vazamentos.
- A rota oficial fixada por fetch não configurou neste ambiente porque `github.com` não foi resolvido. É limitação de rede do ambiente, não falha atribuída ao código; a rota externa compilou, mas sua versão não foi comprovada.
- A instalação externa da FMILibrary não é baseline: esta compilação não comprovou versão, compatibilidade ou execução de FMU.
- Nenhuma FMU foi executada. Não houve validação de Raspberry Pi, USB, ROS, firmware, Qt, bancada, HIL, eletricidade, deadline ou timing real.
- O terminal de debug e `printf`/`fprintf` legados na thread de simulação/FMU continuam. O módulo novo de logging retorna diagnóstico estruturado e não faz I/O textual no produtor, mas o requisito final do caminho crítico permanece parcial.
- A capacidade 128 da fila é fixa no código e precisa ser dimensionada por medição. O agendamento atual não comprova a política final de grade fixa; TASK-004 mantém essa responsabilidade.

## Resultado do motor onp-spec

Em 09.09.2026, os comandos foram executados fora do sandbox restrito porque o motor inicia o processo-filho do harness. `verify host-build-foundation` registrou 8/8 critérios PASS, `verify sdd-versioning` 6/6 e `verify host-run-logging` 12/12. O comando `audit --ci` terminou com `auditoria limpa (0 aviso(s))`. Os três arquivos em `.spec/verification/` registram o resultado e o commit de base `d6c220f` usado na verificação.
