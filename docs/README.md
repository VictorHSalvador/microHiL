# Documentação de desenvolvimento do MICROHIL

Esta é a entrada da documentação de trabalho. Os Markdown orientam a retomada incremental do software existente. As respostas DEC-001…012 e Q-01…09 foram consolidadas; decisões de produto estão registradas e parâmetros dependentes de medição permanecem explícitos. A implementação aguarda somente a verificação cruzada final desta revisão documental.

## Leitura e responsabilidade

| Documento | Responsabilidade |
|---|---|
| [sdd-versions.md](sdd-versions.md) | Versão vigente, histórico e procedimento de atualização do conjunto SDD |
| [constitution.md](constitution.md) | Regras permanentes, comentários e convenções de código |
| [context.md](context.md) | Objetivo, fronteiras, fontes e situação do protótipo |
| [TARGET.md](TARGET.md) | Hardware fornecido, reservas e lacunas do perfil físico |
| [spec.md](spec.md) | Especificação de trabalho, 53 IDs de origem preservados e seis derivados, com critérios de aceitação propostos |
| [architecture.md](architecture.md) | Arquitetura atual, evolução, estados e concorrência |
| [contracts/interfaces.md](contracts/interfaces.md) | Contratos internos e ICD host–DAQC vigente |
| [decisions.md](decisions.md) | Decisões confirmadas e parâmetros restantes de implementação/ensaio |
| [adrs/ADR-001-coding-style.md](adrs/ADR-001-coding-style.md) | Decisão confirmada de nomenclatura e comentários |
| [adrs/ADR-002-product-decisions.md](adrs/ADR-002-product-decisions.md) | Decisões de produto confirmadas pelas respostas DEC |
| [plan.md](plan.md) | Backlog ordenado, dependências, tarefas e critérios de conclusão |
| [tasks/TASK-001.md](tasks/TASK-001.md) | Fundação de build HOST implementada, evidenciada e auditada |
| [traceability.md](traceability.md) | Requisito → design → tarefa → teste → evidência |
| [verification/verification-plan.md](verification/verification-plan.md) | Ambientes, procedimentos e limites dos testes |
| [quality-gates.md](quality-gates.md) | Gates por escopo, com o resultado demonstrado da fundação HOST |
| [evidence/host-build-foundation-2026-09-07.md](evidence/host-build-foundation-2026-09-07.md) | Ambiente, comandos, resultados e limites executados da TASK-001 |

## Fontes e precedência

1. Instruções explícitas do usuário nesta sessão, inclusive a alteração das convenções C/Python.
2. [Especificação de trabalho](spec.md), derivada dos requisitos recebidos; alterações confirmadas são identificadas e propostas não substituem exigências sem decisão.
3. Contexto, alvo, arquitetura, contratos e decisões, cada um dentro de sua função. Contratos incompletos não são liberados para firmware.
4. [Requisitos originais recebidos](references/MICROHIL-REQ-001-A.md) e [descrição ESP32 recebida](references/ESP32-CONTROLADOR.md), preservados byte a byte como referências históricas.
5. [Avaliação inicial](avaliacao-sdd-2026-09-06.md) e [evidência HOST inicial](evidence/audit-2026-09-06/README.md), retratos históricos que não são atualizados para ocultar falhas anteriores.

A [conversão Markdown do DOCX de origem](references/MICROHIL-REQ-001-A.md) está preservada nesta pasta; o DOCX não está mais em docs/. Não manter DOCX e Markdown como duas especificações editáveis independentes. A evolução solicitada ocorre em `spec.md`; a cópia da referência A conserva a redação recebida.

## Atualização desta revisão

Baseline vigente: **SDD-MICROHIL 0.7.2**. O histórico central está em [sdd-versions.md](sdd-versions.md); revisões internas preservadas nos documentos continuam úteis, mas não substituem esse registro. A verificação estrutural reproduzível está em [sdd-versioning.json](../.spec/verification/sdd-versioning.json).

Revisões 0.4–0.6 corrigiram direção, retenção no host, proteção por 100 passos, zero físico no encerramento, grade fixa e reuso arquitetural do RaspDAQ. O código de produção permanece preservado.

Os testes de aceitação descritos em `spec.md` e na matriz estão **planejados**. Somente o registro HOST de auditoria contém verificações já executadas. CONOPS, apresentação, foto, log bruto esptool e PDF 3.0 referido no texto ESP32 não foram recebidos nesta etapa.

A [revisão documental desta sessão](evidence/document-review-2026-09-06.md) registra a conferência dos IDs, textos, referências e links. Ela valida a estrutura dos documentos, sem aprovar requisitos de produto ou ensaios ainda pendentes.

A [verificação da revisão 0.2](evidence/decision-review-2026-09-06.md) registra as respostas DEC incorporadas, 58 requisitos rastreados e a inspeção adicional de start na FMU. Não contém teste de execução ou validação de hardware.

A [revisão Q e inspeção RaspDAQ](evidence/q-review-2026-09-06.md) registra as fontes consultadas e a verificação documental de 59 requisitos. Nenhum serviço/firmware foi executado nesta revisão.

A [verificação da correção de direção](evidence/direction-review-2026-09-06.md) registra a revisão 0.4. Evidências anteriores são históricas e não substituem os requisitos corrigidos.

Revisão 0.6: leitor USB em thread independente; consumir última atualização antes da inserção FMI; ausência conta timeout separado. Encerrar simulação zera AO/DO/PWM e encerra DATA, enquanto novo Play reaplica outputs iniciais da FMU reinicializada. Aguardar próximo instante da grade fixa após overrun, sem compensar nem alterar passos do modelo; relatório temporal somente no fim. ADC/PWM configuráveis dentro das opções explícitas do TARGET.

[Verificação da revisão 0.5](evidence/config-timing-review-2026-09-06.md): configuração ADC/PWM, encerramento físico e grade fixa sem compensação.

[ADR-003: reuso RaspDAQ](adrs/ADR-003-raspdaq-icd.md) e [verificação 0.6](evidence/raspdaq-icd-review-2026-09-06.md): base do ICD consolidada pela referência; a evidência 0.6 permanece histórica.

Revisão 0.7: DATA real-time sem CRC, retransmissão ou sessão no fio; gaps detectáveis são contados e o pacote novo segue. READ_ACK MID 03 confirma consumo sem solicitar replay; XRCE MID 04 integra micro-ROS sob o mesmo dono do enlace. A [evidência de consolidação 0.7](evidence/realtime-loss-policy-review-2026-09-07.md) registra a verificação documental, sem alegar teste do produto.

Patch 0.7.2: a [TASK-001](tasks/TASK-001.md) registrou build limpo independente, CTest, diagnóstico da dependência e builds do runner com a revisão oficial e com instalação externa. A [evidência correspondente](evidence/host-build-foundation-2026-09-07.md) delimita que nenhuma FMU, Raspberry Pi, DAQC, USB, ROS, GUI, concorrência real SPSC ou deadline foi validado; o verify comprovou 8/8 critérios da feature e a auditoria final ficou limpa.
