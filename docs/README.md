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
| [guides/demo-loopback-gui.md](guides/demo-loopback-gui.md) | Tutorial da demonstração física pela GUI: Agent, build, jumpers, FMU/YAML e gráficos |
| [evidence/host-build-foundation-2026-09-07.md](evidence/host-build-foundation-2026-09-07.md) | Ambiente, comandos, resultados e limites executados da TASK-001 |
| [evidence/host-run-logging-2026-09-08.md](evidence/host-run-logging-2026-09-08.md) | Ambiente, comandos, resultados e limites executados da TASK-002 |
| [evidence/micro-ros-host-baseline-2026-09-09.md](evidence/micro-ros-host-baseline-2026-09-09.md) | Build HOST do setup/Agent Humble e incompatibilidade observada com a baseline ESP-IDF 4.4.8 |
| [evidence/micro-ros-agent-persistent-2026-09-19.md](evidence/micro-ros-agent-persistent-2026-09-19.md) | Reconstrução persistente do Agent e escuta UDP 8888 após reinicialização |
| [evidence/host-gui-local-file-url-2026-09-19.md](evidence/host-gui-local-file-url-2026-09-19.md) | Correção e teste da conversão de URL local do seletor de arquivos Qt |
| [evidence/host-ros-setup-config-2026-09-10.md](evidence/host-ros-setup-config-2026-09-10.md) | Build HOST das interfaces ROS ampliadas para configuração ADC/PWM |
| [evidence/esp32-ch340-config-smoke-2026-09-11.md](evidence/esp32-ch340-config-smoke-2026-09-11.md) | Gravação física ESP32 e ensaio seguro CONFIG ENABLE→DISABLE pela CH340 |
| [evidence/esp32-xrce-agent-smoke-2026-09-11.md](evidence/esp32-xrce-agent-smoke-2026-09-11.md) | Ensaio físico XRCE/Agent e limitação atual da sessão |
| [evidence/esp32-ros-supervisor-diagnostic-2026-09-13.md](evidence/esp32-ros-supervisor-diagnostic-2026-09-13.md) | Correção diagnóstica do início ROS fora de `app_main`; ensaio CONFIG desta imagem pendente |
| [evidence/esp32-config-retry-smoke-2026-09-13.md](evidence/esp32-config-retry-smoke-2026-09-13.md) | CONFIG confirmado com espera pós-reset e tentativas limitadas; 0.26.7 gravada |
| [evidence/esp32-xrce-bridge-smoke-2026-09-13.md](evidence/esp32-xrce-bridge-smoke-2026-09-13.md) | Ponte MID 04 com Agent UDP e nó ROS observados em DISABLE |
| [evidence/esp32-harness-rx-fix-2026-09-13.md](evidence/esp32-harness-rx-fix-2026-09-13.md) | Correção do acúmulo RX do harness antes do parser CONFIG |
| [evidence/esp32-streaming-smoke-2026-09-13.md](evidence/esp32-streaming-smoke-2026-09-13.md) | Ensaio físico de transição para STREAMING, aquisição periódica a ~96,4 Hz e confirmação READ_ACK |
| [evidence/esp32-streaming-harness-safe-stop-2026-09-13.md](evidence/esp32-streaming-harness-safe-stop-2026-09-13.md) | `finally` do harness solicita DISABLE depois de STREAMING, inclusive em falha de coleta |
| [evidence/esp32-watchdog-no-ack-2026-09-13.md](evidence/esp32-watchdog-no-ack-2026-09-13.md) | Ensaio físico do watchdog após 60 s sem avanço de READ_ACK |
| [evidence/esp32-actuation-loopback-2026-09-13.md](evidence/esp32-actuation-loopback-2026-09-13.md) | Ensaio físico DATA host→DAQC com loopbacks DO, DAC e PWM em nível alto |
| [evidence/esp32-ros-state-confirmation-2026-09-13.md](evidence/esp32-ros-state-confirmation-2026-09-13.md) | Confirmação passiva de perfil/configuração aplicada em `/daqc_state` durante o procedimento físico |
| [evidence/host-runner-daqc-preflight-2026-09-13.md](evidence/host-runner-daqc-preflight-2026-09-13.md) | Preflight C interrompido sem Agent local ativo; métricas seriais/XRCE registradas |
| [evidence/host-runner-daqc-fmu-2026-09-14.md](evidence/host-runner-daqc-fmu-2026-09-14.md) | Ciclo físico do runner C com Agent, DAQC, STREAMING, 20 passos FMI e DISABLE; valores elétricos por canal não registrados |
| [evidence/physical-fmu-loopback-2026-09-14.md](evidence/physical-fmu-loopback-2026-09-14.md) | Loopback GPIO25→GPIO32 pelo runner C durante 200 passos e retenção das faixas AO/PWM no host |
| [evidence/physical-fmu-loopback-range-guard-2026-09-14.md](evidence/physical-fmu-loopback-range-guard-2026-09-14.md) | Revalidação do loopback após proteção de faixa, com observação tipada de `/daqc_errors` |
| [evidence/host-gui-hil-integration-2026-09-14.md](evidence/host-gui-hil-integration-2026-09-14.md) | Build da GUI ROS com Play HiL no mesmo processo e limites do ensaio HOST |
| [evidence/gui-three-loopback-2026-09-14.md](evidence/gui-three-loopback-2026-09-14.md) | Play HiL físico de 500 passos pela GUI com loopbacks digital, DAC/ADC e observação PWM/ADC |
| [evidence/host-gui-profile-mapping-reflection-2026-09-14.md](evidence/host-gui-profile-mapping-reflection-2026-09-14.md) | Correção HOST que reflete na GUI os canais, escalas e offsets do YAML carregado |

## Fontes e precedência

1. Instruções explícitas do usuário nesta sessão, inclusive a alteração das convenções C/Python.
2. [Especificação de trabalho](spec.md), derivada dos requisitos recebidos; alterações confirmadas são identificadas e propostas não substituem exigências sem decisão.
3. Contexto, alvo, arquitetura, contratos e decisões, cada um dentro de sua função. Contratos incompletos não são liberados para firmware.
4. [Requisitos originais recebidos](references/MICROHIL-REQ-001-A.md) e [descrição ESP32 recebida](references/ESP32-CONTROLADOR.md), preservados byte a byte como referências históricas.
5. [Avaliação inicial](avaliacao-sdd-2026-09-06.md) e [evidência HOST inicial](evidence/audit-2026-09-06/README.md), retratos históricos que não são atualizados para ocultar falhas anteriores.

A [conversão Markdown do DOCX de origem](references/MICROHIL-REQ-001-A.md) está preservada nesta pasta; o DOCX não está mais em docs/. Não manter DOCX e Markdown como duas especificações editáveis independentes. A evolução solicitada ocorre em `spec.md`; a cópia da referência A conserva a redação recebida.

## Atualização desta revisão

Baseline vigente: **SDD-MICROHIL 0.29.9**. O histórico central está em [sdd-versions.md](sdd-versions.md); revisões internas preservadas nos documentos continuam úteis, mas não substituem esse registro. A verificação estrututal reproduzível está em [sdd-versioning.json](../.spec/verification/sdd-versioning.json).

Revisão 0.29.2: a malha física de 200 passos foi repetida depois da proteção HOST de faixa, com `SCHED_FIFO`, zero deadline perdido e assinante tipado de `/daqc_errors` ativo. Nenhuma mensagem foi observada nesse tópico durante o ensaio; isso não substitui a propagação dos erros assíncronos ao resultado da aplicação nem qualificação elétrica ou temporal. Revisão 0.29.3: o lifecycle DAQC tornou-se um módulo C compartilhado e a GUI o chama por tarefa Qt concorrente no mesmo processo. Revisão 0.29.4: a GUI completou 500 passos físicos com FMU de três loopbacks e confirmou digital, DAC/ADC e observação PWM/ADC; duty PWM intermediário continua pendente de filtro RC. Revisão 0.29.6: URLs locais vindas dos diálogos Qt são convertidas pelo controlador antes do uso. Revisão 0.29.7: o parser e o gravador YAML usam ponto decimal de forma independente da localidade; o teste Qt reproduziu a falha anterior e passou sob `pt_BR.UTF-8`. Revisão 0.29.8: o procedimento validado foi consolidado em um tutorial de demonstração. Revisão 0.29.9: o tutorial usa uma raiz configurável, descreve os seis gráficos e configura a repetição para 60 s. A repetição física de 60 s a partir do tutorial permanece pendente.

Revisões 0.26.0–0.26.4: a configuração YAML aceita `acquisition.frequency_hz`; novos perfis GUI iniciam em 100 Hz e o limite vigente é 1…100 Hz, compatível com o tick FreeRTOS de 10 ms configurado. A frequência configura a tarefa de aquisição da DAQC fora do ciclo FMI e não altera o passo da FMU. A [evidência HOST](evidence/host-configurable-acquisition-2026-09-12.md) cobre a geração da interface ROS e 48 testes; a [evidência ESP-IDF](evidence/esp32-acquisition-build-2026-09-12.md) confirma a regeneração do firmware atual; a [evidência física](evidence/esp32-acquisition-config-smoke-2026-09-12.md) confirma a gravação e CONFIG DISABLE. O ensaio da frequência, Agent e I/O continuam pendentes. A revisão 0.26.2 também corrigiu o contexto que ainda chamava essa configuração persistida de binária.

Revisão 0.26.6: a comunicação UART deixa de depender de uma conversão de milissegundos inferior ao tick FreeRTOS. Quando não houver bytes, a tarefa aguarda uma notificação one-shot de `esp_timer`; o callback não interpreta nem transmite dados. A espera normal é um detalhe de implementação de 1 ms, abaixo do limite de transferência de 5 ms. A guarda de 10 ms só cobre notificação perdida; se for atingida ou o timer não puder ser armado, uma flag local é retida e a tarefa cede um tick para não monopolizar o núcleo. O harness físico passou a usar reset de aplicação, `DATA` de 33 bytes, limite XRCE de 128 bytes e verificação obrigatória de CONFIG DISABLE. Os builds e testes desta revisão passaram e a imagem foi gravada com hash confirmado, mas o ensaio posterior não recebeu a confirmação CONFIG; [a evidência](evidence/esp32-xrce-scheduling-2026-09-13.md) limita esse resultado e não declara XRCE funcional.

Revisão 0.26.7: a inspeção da imagem sem resposta CONFIG observou que `app_main`, com pilha configurada de 4096 bytes, ainda executava `DaqcRosStart` de forma síncrona. A inicialização micro-ROS passa a ocorrer somente pela supervisora no núcleo 0, criada após a tarefa UART com pilha de 8192 bytes; ela mantém a tentativa a cada segundo apenas fora de STREAMING. Esta é uma correção diagnóstica, não a causa confirmada da ausência de resposta. A evidência registra testes HOST e exige gravar esta imagem e obter CONFIG DISABLE pela CH340 antes de inferir resultado de XRCE ou ROS.

Revisão 0.26.8: a imagem 0.26.7 foi compilada, gravada e teve hashes confirmados. Após reset, espera de 1 s e cinco CONFIG DISABLE a cada 200 ms, houve cinco confirmações `5972010101`; sem reset, houve nove confirmações em dez tentativas. MID 04 de inicialização apareceu nessa janela. O falso bloqueio anterior vinha de uma única tentativa após somente 200 ms. O harness agora usa espera pós-reset, tentativas e timeout total configuráveis e limitados; o resultado confirma CONFIG nesse procedimento, sem validar XRCE, Agent, tópicos, DATA, I/O, timing ou HIL.

Revisão 0.26.9: o harness passou a abrir a CH340 antes de ajustar DTR/RTS, igual ao procedimento manual que confirmou CONFIG, e usa timeout de leitura de 20 ms. Em ponte temporária de 8 s, com a DAQC em DISABLE e Agent Humble UDP local, houve 468 frames MID 04 DAQC→Agent e 285 Agent→DAQC; foram observados o nó `/microhil_daqc` e os tópicos `/daqc_errors`, `/daqc_setup` e `/daqc_state`. Não houve STREAMING, DATA ou I/O físico.

Revisão 0.26.10: `read_disable_confirmation()` lia bytes da UART, mas não os adicionava ao buffer consumido pelo parser. O harness agora acumula cada chunk antes de extrair frames, corrigindo o falso negativo CONFIG mesmo quando a resposta chega fragmentada. A UART já havia respondido no procedimento manual; esta correção é no diagnóstico HOST e não altera firmware, protocolo ou os limites do ensaio MID 04.

Revisão 0.26.11: validação física do ciclo completo de estados e aquisição periódica no ESP32 físico via `tools/esp32_streaming_smoke.py`. Foi demonstrada a rejeição de segurança ao tentar entrar em STREAMING sem configuração prévia (`REQ-F-02`/`F-18`), e o sucesso após publicação de `DaqcSetup` via ROS 2: transição para STREAMING, recepção de 289 frames de aquisição DATA (33 bytes) a ~96,4 Hz contendo DI e AI calibrados em Volts, com envio de confirmação `READ_ACK` para cada frame (`REQ-F-27`), seguido de DISABLE confirmado e cessação de DATA; não houve medição elétrica das saídas físicas.

Revisão 0.26.12: a auditoria corrigiu a rastreabilidade do ensaio 0.26.11 e delimitou a evidência. A cessação de DATA e a confirmação CONFIG DISABLE foram observadas; o firmware solicita o nível seguro, mas não houve medição elétrica de tensão ou duty cycle de saída. Os valores analógicos observados são a conversão por line fitting do ESP-IDF, não uma calibração de bancada.

Revisão 0.26.13: o harness de STREAMING passou a solicitar DISABLE em `finally` após a aceitação de STREAMING, inclusive se a coleta falhar. A mudança é verificada no HOST e não substitui ensaio elétrico.

Revisão 0.26.14: o ensaio físico sem READ_ACK observou 5.983 frames DATA em 59,98 s. O último frame chegou a 60,00 s após o corte do ACK; não houve novo DATA nos 5 s restantes da observação, antes do cleanup confirmar DISABLE. Isso confirma o watchdog de progresso de leitura nesta placa e firmware, sem comprovar atuação física nem HIL.

Revisão 0.26.15: o ensaio físico de DATA host→DAQC confirmou, por jumpers locais, DO GPIO16 em DI GPIO4, DAC GPIO25 em AI GPIO32 (1,6715 V mediano) e PWM GPIO18 em AI GPIO33 com duty 1,0 (3,118 V mediano). A aplicação de atuação passou da tarefa de comunicação para a tarefa de I/O no núcleo 1. O ensaio não caracteriza duty intermediário, precisão, carga, coordenador C ou HIL.

Revisão 0.26.16: o runner terminal encerra ao receber EOF em vez de reapresentar o menu indefinidamente. Revisão 0.26.17: o artefato ROS do runner declara caminho de bibliotecas para funcionar com `CAP_SYS_NICE`. Revisão 0.26.18: esse caminho passa a usar `DT_RPATH` transitivo, pois `DT_RUNPATH` não encontrou a dependência indireta `librcl_yaml_param_parser.so` sob o carregador seguro. Revisão 0.27.0: antes de publicar `DaqcSetup`, o runner espera 6.000 ms configuráveis para estabilização do Agent/XRCE; a confirmação de CONFIG permanece 10 ms e a confirmação ROS posterior permanece 100 ms. Essa espera antecede a thread FMI. A confirmação de `/daqc_state` foi observada passivamente pelo host após o procedimento físico do harness. Revisão 0.27.1: o Agent e a DAQC foram confirmados pelo harness Python, enquanto a ponte C transmitiu XRCE sem receber resposta; a causa interna permaneceu pendente naquela revisão. Revisão 0.27.2: os primeiros payloads XRCE dos dois caminhos foram preservados como evidência, sem atribuir causa. Revisão 0.28.0: o runner C passou a descartar RX pendente e confirmar DISABLE antes de ENABLE. Revisão 0.29.0: a saída Agent→DAQC passou a usar FIFO limitada e independente de CONFIG; o preflight ganhou reset RTS configurável e o runner C completou uma execução física de 20 passos com Agent, DAQC e FMU. Revisão 0.29.1: uma execução de 200 passos mostrou a evolução do loopback GPIO25→GPIO32; o host passou a reter o último AO/PWM dentro da faixa física. Precisão elétrica e qualificação temporal continuam pendentes.

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

Revisão 0.22: o `fmu_rt_runner` será um nó ROS 2 em C por `rcl`. A publicação e recepção ROS ocorrem em thread de controle própria, sem chamadas FMI, acesso à TTY ou espera no ciclo HiL. O Play aguarda a confirmação de configuração da DAQC antes de solicitar STREAMING e criar a thread de simulação.

Patch 0.22.1: o componente ROS do runner foi materializado e compilado com ROS 2 Humble. Ele inicia a thread `rcl`, publica `DaqcSetup` e armazena `DaqcState`/`DaqcErrors`, mas ainda não está ligado ao ciclo de Play porque o prazo de confirmação ROS precisa ser definido. A [evidência](evidence/host-rcl-control-2026-09-10.md) limita o resultado ao HOST.

Revisão 0.23: a confirmação ROS de `DaqcSetup` inicia em 100 ms e permanece configurável, enquanto CONFIG UART crítico continua com 10 ms. O prazo ROS ocorre uma vez antes de STREAMING e não participa do orçamento FMI.

Patch 0.23.1: em modo DAQC, o runner valida SCHED_FIFO antes de abrir STREAMING, prepara o coordenador/TTY/ponte UDP/nó ROS, confirma ENABLE e a configuração pelo `DaqcState`, e só então inicia a thread FMI. Ao encerrar, solicita DISABLE e encerra os workers. A [evidência](evidence/host-runner-daqc-lifecycle-2026-09-10.md) cobre build e testes HOST, sem declarar que o enlace físico foi exercitado.
