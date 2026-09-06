# Documentação de desenvolvimento do MICROHIL

Esta é a entrada da documentação de trabalho. Os Markdown orientam a retomada incremental do software existente. A base está **em elaboração**, com requisitos recebidos preservados e decisões pendentes explícitas. As respostas DEC-001…012 já definem várias escolhas; dúvidas restantes estão identificadas por Q-01…09. Implementação aguarda consolidação documental por instrução explícita do usuário.

## Leitura e responsabilidade

| Documento | Responsabilidade |
|---|---|
| [constitution.md](constitution.md) | Regras permanentes, comentários e convenções de código |
| [context.md](context.md) | Objetivo, fronteiras, fontes e situação do protótipo |
| [TARGET.md](TARGET.md) | Hardware fornecido, reservas e lacunas do perfil físico |
| [spec.md](spec.md) | Especificação de trabalho, 53 IDs de origem preservados e cinco derivados, com critérios de aceitação propostos |
| [architecture.md](architecture.md) | Arquitetura atual, evolução, estados e concorrência |
| [contracts/interfaces.md](contracts/interfaces.md) | Contratos internos e campos pendentes do ICD host–DAQC |
| [decisions.md](decisions.md) | Questões, alternativas, impacto e quem precisa responder |
| [adrs/ADR-001-coding-style.md](adrs/ADR-001-coding-style.md) | Decisão confirmada de nomenclatura e comentários |
| [adrs/ADR-002-product-decisions.md](adrs/ADR-002-product-decisions.md) | Decisões de produto confirmadas pelas respostas DEC |
| [plan.md](plan.md) | Backlog ordenado, dependências, tarefas e critérios de conclusão |
| [tasks/TASK-001.md](tasks/TASK-001.md) | Primeiro incremento de implementação após a etapa documental |
| [traceability.md](traceability.md) | Requisito → design → tarefa → teste → evidência |
| [verification/verification-plan.md](verification/verification-plan.md) | Ambientes, procedimentos e limites dos testes |
| [quality-gates.md](quality-gates.md) | Gates por escopo, sem bloquear trabalho independente |

## Fontes e precedência

1. Instruções explícitas do usuário nesta sessão, inclusive a alteração das convenções C/Python.
2. [Especificação de trabalho](spec.md), derivada dos requisitos recebidos; alterações confirmadas são identificadas e propostas não substituem exigências sem decisão.
3. Contexto, alvo, arquitetura, contratos e decisões, cada um dentro de sua função. Contratos incompletos não são liberados para firmware.
4. [Requisitos originais recebidos](references/MICROHIL-REQ-001-A.md) e [descrição ESP32 recebida](references/ESP32-CONTROLADOR.md), preservados byte a byte como referências históricas.
5. [Avaliação inicial](avaliacao-sdd-2026-09-06.md) e [evidência HOST inicial](evidence/audit-2026-09-06/README.md), retratos históricos que não são atualizados para ocultar falhas anteriores.

A [conversão Markdown do DOCX de origem](references/MICROHIL-REQ-001-A.md) está preservada nesta pasta; o DOCX não está mais em docs/. Não manter DOCX e Markdown como duas especificações editáveis independentes. A evolução solicitada ocorre em `spec.md`; a cópia da referência A conserva a redação recebida.

## Atualização desta revisão

Revisão 0.2 incorpora: perfil ESP32/ADC-DAC internos, futuro Pi 4/2 GB, Qt desacoplado/Mint/debug, log binário de saídas/CSV posterior, retenção/checkbox de inválidos, 100 Hz como meta/timeout até 5 ms/continuar após overrun, gráfico até 10 Hz com ciclo de vida definido e novo protocolo 0x7259. Status/bytes/tamanho, micro-ROS/frames próprios, boot e proteção ainda têm dúvidas localizadas. Achados de código anteriores permanecem abertos; nenhum código de produção foi modificado.

Os testes de aceitação descritos em `spec.md` e na matriz estão **planejados**. Somente o registro HOST de auditoria contém verificações já executadas. CONOPS, apresentação, foto, log bruto esptool e PDF 3.0 referido no texto ESP32 não foram recebidos nesta etapa.

A [revisão documental desta sessão](evidence/document-review-2026-09-06.md) registra a conferência dos IDs, textos, referências e links. Ela valida a estrutura dos documentos, sem aprovar requisitos de produto ou ensaios ainda pendentes.

A [verificação da revisão 0.2](evidence/decision-review-2026-09-06.md) registra as respostas DEC incorporadas, 58 requisitos rastreados e a inspeção adicional de start na FMU. Não contém teste de execução ou validação de hardware.
