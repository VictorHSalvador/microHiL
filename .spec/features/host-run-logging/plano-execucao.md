# Plano de execução — host-run-logging

> gerado por `onp-spec plano` em 2026-09-08 01:05 — NÃO edite à mão;
> mudou tasks.md ou a config? Regenere: `onp-spec plano host-run-logging --modelo gpt-5.6-terra --esforco high`

## Resumo — o que vai acontecer

- **5 tarefa(s) pendente(s)**: 5 em 2 faixa(s) paralela(s) + 0 sequencial(is)
- **1 faixa = 1 worktree + 1 branch + 1 janela de contexto limpa** — faixas não compartilham nenhum arquivo entre si
- prefere outra seleção ou uma após a outra? Regenere com `onp-spec plano host-run-logging --paralelizar T-xxx,T-yyy` ou `--sequencial`
- **custo travado pelo usuário**: modelo `gpt-5.6-terra` · esforço `high` em TODAS as tarefas (vence tasks.md e config)
- tudo acontece na branch de trabalho `spec/host-run-logging`; levar para a main é decisão sua

## Faixas e ondas

### Onda 1 — faixa-1 ∥ faixa-2

#### faixa-1 — branch `spec/host-run-logging-faixa-1` — worktree `../onp-worktrees/microHiL-host-run-logging-faixa-1`

| tarefa | título | modelo | esforço | arquivos |
|---|---|---|---|---|
| T-005 | Implementar e testar o codec binário tipado | `gpt-5.6-terra` | high | `include/log_format.h`, `src/log_format.c`, `tests/test_log_format.c`, `tests/CMakeLists.txt`, `CMakeLists.txt` |
| T-006 | Implementar o logger assíncrono e os testes concorrentes | `gpt-5.6-terra` | high | `include/binary_logger.h`, `src/binary_logger.c`, `tests/test_binary_logger.c`, `tests/test_sample_queue.c`, `tests/CMakeLists.txt`, `CMakeLists.txt` |
| T-007 | Implementar e testar a conversão pós-execução para CSV | `gpt-5.6-terra` | high | `include/log_converter.h`, `src/log_converter.c`, `tests/test_log_converter.c`, `tests/CMakeLists.txt`, `CMakeLists.txt` |
| T-008 | Integrar logging binário ao runner e ao resultado final | `gpt-5.6-terra` | high | `include/app_config.h`, `include/common.h`, `include/rt_simulation.h`, `src/app_config.c`, `src/main.c`, `src/rt_simulation.c`, `src/csv_logger.c`, `include/csv_logger.h`, `CMakeLists.txt`, `tests/test_run_logging.c`, `tests/CMakeLists.txt`, `test/run_spec_tests.js` |

#### faixa-2 — branch `spec/host-run-logging-faixa-2` — worktree `../onp-worktrees/microHiL-host-run-logging-faixa-2`

| tarefa | título | modelo | esforço | arquivos |
|---|---|---|---|---|
| T-009 | Atualizar SDD, rastreabilidade e evidência da TASK-002 | `gpt-5.6-terra` | high | `README.md`, `docs/README.md`, `docs/contracts/interfaces.md`, `docs/plan.md`, `docs/tasks/TASK-002.md`, `docs/traceability.md`, `docs/quality-gates.md`, `docs/sdd-versions.md`, `docs/evidence/host-run-logging-2026-09-07.md`, `.spec/features/host-run-logging/spec.md`, `.spec/features/host-run-logging/tasks.md`, `test/sdd-versioning.spec.test.js`, `onpspec.config.json` |

## Gestão de branches e commits

1. branch de trabalho `spec/host-run-logging` criada do ponto atual (se ainda não existir)
2. cada faixa nasce dela como branch própria e roda no seu worktree — **1 tarefa = 1 commit** (`T-xxx feature: título`)
3. terminou a onda → merge `--no-ff` de cada faixa de volta, na ordem; conflito interrompe a faixa e pede resolução humana
4. faixa mesclada → worktree removido, branch apagada, tarefa marcada `[concluida]` no tasks.md
5. gate final na branch de trabalho: `onp-spec verify host-run-logging` + `onp-spec audit --ci` — **exit 0 ou não está pronto**

## Como executar

### ▶ Execução — Codex headless (codex exec)

```bash
bash .spec/features/host-run-logging/executar-tarefas.sh
```

Cada faixa roda `codex exec` com **janela de contexto limpa**, no seu worktree, com
`--model` e `model_reasoning_effort` já definidos por tarefa e sandbox `workspace-write`. Os prompts exatos estão
embutidos no script — quer rodar uma faixa na mão, é só copiá-los de lá.
Logs: `../onp-worktrees/microHiL-host-run-logging-logs/`.

**Confirmação de custos — antes de executar**: os modelos e esforços por
tarefa estão nas tabelas acima; o agente CONFIRMA com o usuário se estão
dentro da licença/cota dele (modelo forte + esforço alto torra tokens).
Para gastar menos: `onp-spec plano host-run-logging --modelo gpt-5.6-luna --esforco baixo`
(tudo) ou por tarefa `onp-spec tarefa host-run-logging T-xxx --modelo <m> --esforco <nível>` — e regenere o plano.

### 📣 Acompanhamento — tabela + resumo no chat (a cada 1 min)

O script roda em **background**: o agente AVISA o usuário antes de iniciar e,
enquanto roda, posta no chat a cada ~1 minuto a **tabela de andamento** (qual
tarefa está rodando, qual não está, o que concluiu/falhou) junto com o
**resumo geral de andamento** (escrito por IA; sem IA, o motor resume). Ao
final, o usuário recebe o resumo completo da execução. A qualquer momento:

```bash
onp-spec resumo host-run-logging --tabela   # a tabela de andamento
onp-spec resumo host-run-logging            # o resumo em texto
```

