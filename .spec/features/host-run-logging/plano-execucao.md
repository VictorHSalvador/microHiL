# Plano de execução — host-run-logging

> gerado por `onp-spec plano` em 2026-09-08 01:46 — NÃO edite à mão;
> mudou tasks.md ou a config? Regenere: `onp-spec plano host-run-logging --sequencial --modelo gpt-5.6-terra --esforco high`

## Resumo — o que vai acontecer

- **modo SEQUENCIAL (escolha do usuário)**: 5 tarefa(s) pendente(s), UMA APÓS A OUTRA, na árvore principal
- sem worktrees e sem paralelismo — cada tarefa roda numa janela de contexto limpa, na ordem do tasks.md
- **custo travado pelo usuário**: modelo `gpt-5.6-terra` · esforço `high` em TODAS as tarefas (vence tasks.md e config)
- tudo acontece na branch de trabalho `spec/host-run-logging`; levar para a main é decisão sua

## Ordem de execução (uma tarefa após a outra)

| tarefa | título | modelo | esforço |
|---|---|---|---|
| T-005 | Implementar e testar o codec binário tipado | `gpt-5.6-terra` | high |
| T-006 | Implementar o logger assíncrono e os testes concorrentes | `gpt-5.6-terra` | high |
| T-007 | Implementar e testar a conversão pós-execução para CSV | `gpt-5.6-terra` | high |
| T-008 | Integrar logging binário ao runner e ao resultado final | `gpt-5.6-terra` | high |
| T-009 | Atualizar SDD, rastreabilidade e evidência da TASK-002 | `gpt-5.6-terra` | high |

## Gestão de branches e commits

1. branch de trabalho `spec/host-run-logging` criada do ponto atual (se ainda não existir)
2. as tarefas rodam nela mesma, na ordem — **1 tarefa = 1 commit** (`T-xxx feature: título`), marcada `[concluida]` só com trabalho feito
3. gate final na branch de trabalho: `onp-spec verify host-run-logging` + `onp-spec audit --ci` — **exit 0 ou não está pronto**

## Como executar

### ▶ Execução — Codex headless (codex exec)

```bash
bash .spec/features/host-run-logging/executar-tarefas.sh
```

Cada tarefa roda `codex exec` com **janela de contexto limpa**, na árvore principal,
uma após a outra, com `--model` e `model_reasoning_effort` já definidos por tarefa e sandbox `workspace-write`.
Os prompts exatos estão embutidos no script.
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

