# Documentação de desenvolvimento do MICROHIL

Esta é a entrada da documentação de trabalho. Os Markdown orientam a retomada incremental do software existente. As respostas DEC-001…012 e Q-01…09 foram consolidadas; decisões de produto estão registradas e parâmetros dependentes de medição permanecem explícitos. A implementação avança por incrementos verificáveis, sem promover resultados HOST a validação de produto.

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
| [tasks/TASK-002.md](tasks/TASK-002.md) | Logging binário e conversão posterior HOST implementados, evidenciados e auditados mecanicamente, sem validação de produto |
| [traceability.md](traceability.md) | Requisito → design → tarefa → teste → evidência |
| [verification/verification-plan.md](verification/verification-plan.md) | Ambientes, procedimentos e limites dos testes |
| [quality-gates.md](quality-gates.md) | Gates por escopo, com o resultado demonstrado da fundação HOST |
| [evidence/host-build-foundation-2026-09-07.md](evidence/host-build-foundation-2026-09-07.md) | Ambiente, comandos, resultados e limites executados da TASK-001 |
| [evidence/host-run-logging-2026-09-08.md](evidence/host-run-logging-2026-09-08.md) | Ambiente, comandos, resultados e limites executados da TASK-002 |
| [evidence/micro-ros-host-baseline-2026-09-09.md](evidence/micro-ros-host-baseline-2026-09-09.md) | Build HOST do setup/Agent Humble e incompatibilidade observada com a baseline ESP-IDF 4.4.8 |
| [evidence/host-ros-setup-config-2026-09-10.md](evidence/host-ros-setup-config-2026-09-10.md) | Build HOST das interfaces ROS ampliadas para configuração ADC/PWM |

## Fontes e precedência

1. Instruções explícitas do usuário nesta sessão, inclusive a alteração das convenções C/Python.
2. [Especificação de trabalho](spec.md), derivada dos requisitos recebidos; alterações confirmadas são identificadas e propostas não substituem exigências sem decisão.
3. Contexto, alvo, arquitetura, contratos e decisões, cada um dentro de sua função. Contratos incompletos não são liberados para firmware.
4. [Requisitos originais recebidos](references/MICROHIL-REQ-001-A.md) e [descrição ESP32 recebida](references/ESP32-CONTROLADOR.md), preservados byte a byte como referências históricas.
5. [Avaliação inicial](avaliacao-sdd-2026-09-06.md) e [evidência HOST inicial](evidence/audit-2026-09-06/README.md), retratos históricos que não são atualizados para ocultar falhas anteriores.

A [conversão Markdown do DOCX de origem](references/MICROHIL-REQ-001-A.md) está preservada nesta pasta; o DOCX não está mais em docs/. Não manter DOCX e Markdown como duas especificações editáveis independentes. A evolução solicitada ocorre em `spec.md`; a cópia da referência A conserva a redação recebida.

## Atualização desta revisão

Baseline vigente: **SDD-MICROHIL 0.21.2**. O histórico central está em [sdd-versions.md](sdd-versions.md); revisões internas preservadas nos documentos continuam úteis, mas não substituem esse registro. A verificação estrutural reproduzível está em [sdd-versioning.json](../.spec/verification/sdd-versioning.json).

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

Patch 0.7.2: a [TASK-001](tasks/TASK-001.md) registrou build limpo independente, CTest, diagnóstico da dependência e builds do runner com a revisão oficial e com instalação externa. A [evidência correspondente](evidence/host-build-foundation-2026-09-07.md) delimita que nenhuma FMU, Raspberry Pi, DAQC, USB, ROS, GUI, concorrência real SPSC ou deadline foi validado.

Patch 0.7.3: a [TASK-002](tasks/TASK-002.md) sincroniza o IF-LOG implementado no HOST: codec `MHILLOG1`, fila SPSC limitada, falhas estruturadas, SHA-256 local e conversão CSV posterior. A [evidência correspondente](evidence/host-run-logging-2026-09-08.md) registra CTest normal e com ASan+UBSan, harness de especificação, compilação externa do runner e os limites: sem execução de FMU, sem qualificação de instalação externa, GUI, Raspberry Pi, USB, ROS, firmware, bancada, HIL ou timing real.

Patch 0.7.4: ESP-IDF v4.4.8 e os ramos Humble de micro-ROS passam a ser a baseline de firmware. A UART CH340–ESP32 é 8N1 configurável entre 9.600 e 115.200 bit/s, com RTS/CTS desabilitado. A decisão documenta uma configuração aprovada; o build integrado e a capacidade de tempo real continuam pendentes de evidência.

Revisão 0.8: o coordenador C passa a possuir exclusivamente a CH340 pela API serial POSIX em `/dev/ttyUSB*`. Ele entrega XRCE do MID 04 a um transporte customizado do Micro-ROS Agent; o Agent serial padrão não abre a porta. A GUI do produto será Qt Quick/QML. Esta revisão muda o contrato de transporte e não constitui execução em placa.

Patch 0.8.1: o setup, as mensagens e o Agent micro-ROS dos ramos Humble tiveram build HOST limpo. O componente ESP-IDF Humble clonado declara ensaios em ESP-IDF 5.2…6.0, enquanto DEC-006 fixa 4.4.8. A [evidência](evidence/micro-ros-host-baseline-2026-09-09.md) preserva o resultado e a pendência; nenhum firmware será iniciado sem uma decisão sobre essa compatibilidade.

Revisão 0.9: o usuário substituiu ESP-IDF 4.4.8 por **ESP-IDF v5.2.6**, a tag oficial da menor série declarada como testada pelo componente micro-ROS Humble atual. A verificação local do SDK lista o alvo `esp32`, compatível com o ESP32-D0WDQ5 clássico. O build integrado e a validação na placa continuam necessários.

Revisão 0.10: tópicos e mensagens micro-ROS foram definidos para setup, state e errors.

Revisão 0.11: UART 152.000 bit/s e MTU XRCE de 128 bytes foram selecionados como baseline. A ponte CH340–ESP32 precisa de ensaio; XRCE continua best effort e não comprova deadline.

Patch 0.11.1: a TTY HOST passou a configurar 152.000 bit/s via `termios2`/`BOTHER` e o codec limita XRCE a 128 bytes. A [evidência HOST](evidence/host-uart-152000-2026-09-09.md) registra 39 testes em pseudo-terminal; não é ensaio da CH340 ou do ESP32.

Patch 0.11.2: o pacote `microhil_interfaces` gerou e inspecionou em ROS 2 Humble as mensagens de setup, state e errors. A [evidência](evidence/host-ros-interfaces-2026-09-09.md) não declara firmware, Agent ou tópicos em execução.

Revisão 0.13: a interface ROS fica limitada a setup, state e errors; aquisição e atuação seguem exclusivamente pelo DATA do ICD.

Patch 0.13.1: o worker serial HOST passou a ser o dono único da TTY, separado da simulação. A [evidência](evidence/host-serial-service-2026-09-10.md) registra o round-trip em pseudo-terminal, sem CH340 ou ESP32.

Revisão 0.14: `profile_id` seleciona um perfil compilado na DAQC; o contrato não transfere mapa variável de pinos pelo MID 04. O coordenador HOST passou a manter XRCE de saída em mailbox de 128 bytes, depois de CONFIG, READ_ACK e DATA.

Revisão 0.15: o mapa simultâneo do perfil ESP32 foi confirmado. DATA usa 28 bytes da DAQC ao host e 21 bytes do host à DAQC. O `profile_id` selecionado é 1.

Revisão 0.16: `/daqc_setup` também configura ADC/PWM fora de STREAMING. O estado confirma a aplicação e erros de ADC/PWM usam flags binárias próprias.

Revisão 0.20: o YAML passou a exigir a configuração completa dos seis ADCs e dois PWMs do perfil ESP32. A GUI deverá gerar esses valores de forma explícita; a validação HOST não substitui a confirmação da DAQC.

Revisão 0.21: o MID 04 chega ao Agent Humble por ponte UDP local, sem permitir que o Agent abra a CH340. CONFIG, DATA e READ_ACK permanecem no coordenador UART.

Patch 0.21.2: o runner inicializa a FMU e resolve as referências iniciais dos inputs antes de iniciar a thread de simulação. Assim, a futura conexão DAQC pode publicar aquisições em um estado pronto, enquanto a primeira chamada `doStep` continua ocorrendo somente após a confirmação de STREAMING. A [evidência HOST](evidence/host-rt-preparation-2026-09-10.md) usa uma fixture FMU e não valida enlace ou tempo real.
