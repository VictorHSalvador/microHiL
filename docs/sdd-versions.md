# Versionamento do SDD do MICROHIL

Versão vigente: **SDD-MICROHIL 0.10.0**, de 09.09.2026.

Status da versão: **baseline documental consolidada; TASK-001 e TASK-002 HOST implementadas, evidenciadas e auditadas mecanicamente; validação do produto pendente**.

Este arquivo é o registro único de versões do conjunto SDD. Ele não substitui a [especificação](spec.md), a [arquitetura](architecture.md), o [ICD](contracts/interfaces.md), as [decisões](decisions.md), o [plano](plan.md), a [rastreabilidade](traceability.md) ou o [plano de verificação](verification/verification-plan.md).

## Estado da baseline vigente

| Eixo | Estado em 0.8.0 | Evidência/limite |
|---|---|---|
| Especificação | 59 requisitos únicos; decisões DEC-001…012 e Q-01…09 consolidadas; ICD 0.7 vigente | Verificação estrutural e motor onp-spec; critérios do produto ainda são planejados |
| Implementação | TASK-001 e TASK-002 HOST concluídas; codec/logging/conversão implementados, requisitos de produto seguem parciais ou pendentes | Build independente e runner compilados; não inferir execução de FMU ou conformidade integral |
| Verificação | CTest 23/23 normal e 23/23 ASan+UBSan; harness e critérios da feature executados | Não executa FMU, ROS, USB, firmware, GUI, Raspberry Pi, bancada, HIL ou timing real |

## Esquema de versão

O SDD usa versões MAJOR.MINOR.PATCH antes e depois da primeira baseline 1.0:

- **MAJOR:** muda a autoridade documental, remove requisito aprovado ou altera de forma incompatível a estrutura de rastreabilidade.
- **MINOR:** acrescenta ou altera comportamento de produto, requisito, decisão arquitetural ou contrato de interface.
- **PATCH:** corrige redação, links, rastreabilidade, evidência ou automação documental sem alterar comportamento aprovado.

Revisões internas de um documento podem registrar apenas MAJOR.MINOR quando uma correção PATCH não muda seu conteúdo normativo. A versão deste arquivo identifica a baseline conjunta do repositório.

## Histórico

As versões 0.1.0 a 0.6.0 foram reconstruídas dos históricos internos e evidências existentes. Um mesmo commit pode conter mais de uma revisão documental intermediária; isso é declarado em vez de inventar commits inexistentes.

| Versão | Data | Natureza | Alteração principal | Artefatos afetados | Evidência disponível | Commit |
|---|---|---|---|---|---|---|
| 0.1.0 | 06.09.2026 | MINOR | Importação dos 53 requisitos de origem, constituição, contexto, arquitetura, plano e rastreabilidade iniciais | docs/spec.md e conjunto documental inicial | document-review-2026-09-06 e audit-2026-09-06 | 0fc7aaf, revisão agrupada |
| 0.2.0 | 06.09.2026 | MINOR | Incorporação DEC-001…012, regras de estilo e requisitos derivados iniciais | spec, ADR-001, ADR-002, decisions, TARGET, architecture | decision-review-2026-09-06 | 0fc7aaf, revisão agrupada |
| 0.3.0 | 06.09.2026 | MINOR | Respostas Q, proteção, logging, timeout, PWM e criação de F-27 | spec, decisions, contracts, verification, traceability | q-review-2026-09-06 | f8eeff3, revisão agrupada |
| 0.4.0 | 06.09.2026 | MINOR | Correção da direção aquisição/atuação, retenção no host e supervisão por leitura | spec, architecture, contracts, TARGET | direction-review-2026-09-06 | f8eeff3, revisão agrupada |
| 0.5.0 | 06.09.2026 | MINOR | Último snapshot antes da FMU, zero físico no fim, configuração ADC/PWM e grade temporal fixa | spec, architecture, contracts, TARGET, verification | config-timing-review-2026-09-06 | f8eeff3, revisão agrupada |
| 0.6.0 | 06.09.2026 | MINOR | Inspeção do RaspDAQ e consolidação da base little-endian, CONFIG/DATA, SEQ e ownership | ADR-003, contracts, architecture, decisions | raspdaq-icd-review-2026-09-06 | f8eeff3 |
| 0.7.0 | 07.09.2026 | MINOR | DATA sem CRC/retransmissão/sessão no fio; READ_ACK MID 03; XRCE MID 04; perda segue para o próximo pacote | ADR-003, spec, contracts, architecture, plan, verification | realtime-loss-policy-review-2026-09-07 | 4020de9 |
| 0.7.1 | 07.09.2026 | PATCH | Registro único de versões e auditoria mecânica da governança documental | sdd-versions, README, .spec, teste documental e quality-gates | onp-spec verify/audit desta feature | 61feaa9 |
| 0.7.2 | 07.09.2026 | PATCH | Fundação de build HOST, testes, rastreabilidade e evidência da TASK-001 sem alterar comportamento aprovado do produto | README, TARGET, plano, tarefa, matriz, gates, evidence, .spec e teste documental | host-build-foundation-2026-09-07; verify 8/8; audit limpo | fb04b98 |
| 0.7.3 | 09.09.2026 | PATCH | Sincronização do IF-LOG já aprovado com a implementação/evidência HOST da TASK-002, sem alterar comportamento normativo de produto | README, docs, TASK-002, matriz, gates, evidence, .spec, config e teste documental | host-run-logging-2026-09-08; CTest 23/23 normal/ASan+UBSan; verify 12/12 e audit --ci limpo | a7af739, 742febe, 1e87ac7, d6c220f; documentação nesta baseline |
| 0.7.4 | 09.09.2026 | PATCH | Registro de baseline ESP-IDF 4.4.8/micro-ROS Humble e UART 8N1 configurável de 9.600 a 115.200 bit/s, com RTS/CTS desabilitado | decisions, TARGET, architecture, ICD e plano | Host Ubuntu 22.04.5/ROS Humble observado; build integrado de firmware pendente | bcd172f |
| 0.8.0 | 09.09.2026 | MINOR | Troca autorizada de libusb direto por coordenador C TTY POSIX exclusivo sobre CH340, com XRCE MID 04 entregue ao Agent por transporte customizado; Qt Quick/QML selecionado | spec, decisions, contexto, alvo, arquitetura, ICD, ADRs, plano e verificação | Decisão do usuário; sem execução de CH340, Agent, firmware ou GUI | caeb299 |
| 0.8.1 | 09.09.2026 | PATCH | Evidência de build HOST do setup/Agent micro-ROS Humble e registro da incompatibilidade observada entre o componente Humble atual e ESP-IDF 4.4.8 | decisions, TARGET, plan, evidence e sdd-versions | micro-ros-host-baseline-2026-09-09; firmware/ESP-IDF/placa não executados | alterações locais; registrar commit quando criado |
| 0.9.0 | 09.09.2026 | MINOR | Decisão do usuário de substituir ESP-IDF 4.4.8 por ESP-IDF v5.2.6, série mínima declarada pelo componente micro-ROS Humble atual, para o ESP32 clássico | decisions, TARGET, plan, README e sdd-versions | SDK v5.2.6 instalado no host; `idf.py --list-targets` lista esp32; build integrado e placa pendentes | alterações locais; registrar commit quando criado |
| 0.10.0 | 09.09.2026 | MINOR | Definição dos tópicos micro-ROS, mensagens de setup/state/errors e regra de telemetria por perfil | ICD, architecture, decisions, plan e sdd-versions | Decisão do usuário; MTU XRCE, firmware e perfil físico ainda pendentes | alterações locais; registrar commit quando criado |

## Procedimento de atualização

1. Classificar a mudança como MAJOR, MINOR ou PATCH antes de editar.
2. Atualizar primeiro o artefato responsável: requisito em spec, decisão em ADR/decisions, interface no ICD, alvo em TARGET ou regra permanente na constituição.
3. Sincronizar arquitetura, plano, tarefa, verificação e traceability quando forem afetados.
4. Criar uma nova evidência para verificações executadas; não reescrever evidência histórica para adequá-la à decisão atual.
5. Acrescentar uma linha neste histórico com versão, data, natureza, resumo, artefatos, evidência e commit. Durante trabalho local, usar “alterações locais; registrar commit quando criado” e substituir pelo hash depois do commit.
6. Executar o teste documental, onp-spec verify e onp-spec audit --ci. Registrar falhas como falhas; somente exit code zero libera a versão documental.
7. Atualizar a versão vigente e o índice docs/README.md somente depois de o gate documental passar.

## Autoridade e limites

- [docs/spec.md](spec.md) é a única especificação normativa dos requisitos REQ-F e REQ-NF.
- [.spec/features/sdd-versioning/spec.md](../.spec/features/sdd-versioning/spec.md) especifica apenas a governança mecânica deste registro.
- Aprovação documental não significa implementação, teste de runtime ou validação elétrica/temporal.
- As pastas de evidência registram o que foi realmente executado no ambiente indicado; valores esperados ou didáticos não são promovidos a resultados.
