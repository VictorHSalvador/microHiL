# Decisões e esclarecimentos da especificação

Revisão 0.7. Respostas DEC-001…012 e Q-01…09 incorporadas. **Implementação permanece suspensa somente até concluir e verificar esta consolidação dos Markdown.** A revisão vigente acrescenta a decisão de comunicação real-time: sem CRC, retransmissão ou recuperação de DATA; perda detectável é contabilizada e o fluxo segue para o pacote novo. READ_ACK e XRCE usam MIDs próprios. O histórico anterior permanece nas evidências.

## Decisões de produto

| ID | Confirmado | Detalhamento restante |
|---|---|---|
| DEC-001 | USB-C/CH340 e micro-ROS no ESP32; coordenador C único usa `/dev/ttyUSB*` e transporte customizado do Agent; XRCE MID 04; tópicos `/daqc_setup`, `/daqc_state`, `/daqc_errors` e `/daqc_data` | MTU XRCE 128 bytes selecionado; implementar transporte integrado e medir custo |
| DEC-002 | Qt 6/C++ com Qt Quick/QML desacoplado, terminal debug, prioridade GUI inferior ao núcleo, aparência Mint; log binário tipado e CSV posterior | Validar UX e formatos por fixtures |
| DEC-003 | Perfil ESP32, recursos selecionáveis com exclusão por GPIO e reserva UART; ADC/DAC internos e PWM configurável | Caracterização elétrica continua requisito de bancada |
| DEC-004 | FMU 2.0 CS sem planta fixa, passo/duração configuráveis; meta 100 Hz medida por modelo/alvo; grade fixa, sem compensação; USB ≤5 ms por transferência | Medir orçamento fim a fim por modelo/perfil |
| DEC-005 | Aquisição inválida retida no host, inicial válido do input se necessário; checkbox/100 passos. Encerramento zera saídas físicas e cessa DATA; restart reinicializa FMU | Verificar ordem e confirmação em HIL |
| DEC-006 | Primeiro Linux Ubuntu 22.04.5/ROS 2 Humble; futuro Raspberry Pi 4/2 GB; firmware usa ESP-IDF v5.2.6 e ramos Humble de micro-ROS | Fixar e verificar a baseline integrada no ESP32; kernel/arquitetura do Pi seguem pendentes |
| DEC-007 | Continuar após overrun; último dado válido pode ser reutilizado até fim; saída constante não é defeito; SCHED_FIFO deve ser verificado | Falha de configuração impede Play HiL e gera diagnóstico |
| DEC-008 | Saída da FMU por passo no binário; Real inválido usa NaN, discreto inválido usa zero com bit de qualidade; falha de gravação gera aviso e não interrompe | Formato IF-LOG deve ser testado antes de estabilizar versão 1 |
| DEC-009 | Sem pausa; gráfico até 10 Hz, ticks Y configuráveis, janela temporal comum deslizante; fechar destrói histórico, reabrir começa dali; configuração e abertura separadas, abertura antes de Play permitida | Sem conflito pendente com proteção: ela termina em Error |
| DEC-010 | Preservar APIs Qt; convenção própria C++ e estilo definidos na constituição | Nenhuma decisão de toolkit reaberta |
| DEC-011 | Importação de FMU separada da configuração binária; diagnosticar incompatibilidades específicas com FMU/DAQ/mapa | Pode identificar e informar capacidades não suportadas no incremento inicial (Q-08 resolvida) |
| DEC-012 | SYNC 0x7259; MID CONFIG=01, DATA=02, READ_ACK=03, XRCE=04; COMMAND 01/02/03; payload DATA até 256; STATUS só no CONFIG ESP32→host; sem CRC/retransmissão | Cadência do ACK e prazo agregado CONFIG serão medidos; MTU XRCE é 128 bytes |

## Decisões de toolchain e UART — 0.7.4

O host auditado executa Ubuntu 22.04.5 com `ROS_DISTRO=humble`. A baseline original ESP-IDF v4.4.8 foi substituída por decisão do usuário em 09.09.2026: a baseline de firmware passa a ESP-IDF **v5.2.6**, tag oficial que pertence à série 5.2 selecionada por ser a menor declarada como testada pelo componente micro-ROS Humble atual. O ESP32 clássico é alvo suportado pelo ESP-IDF 5.2 e é listado pelo componente. `micro_ros_setup`, componente ESP-IDF e Agent permanecem nos ramos Humble. A compatibilidade integrada ainda requer build reproduzível e ensaio na placa.

A UART da ponte CH340 é configurável de 9.600 a 152.000 bit/s, com 8N1 e RTS/CTS desabilitado. A baseline selecionada é 152.000 bit/s e deve ser aplicada nos dois extremos antes de STREAMING. Como não é uma taxa POSIX convencional, o host usa `termios2`/`BOTHER` e confirmou a taxa solicitada em pseudo-terminal; CH340–ESP32, taxa efetiva e erro de baud ainda requerem ensaio em placa. O limite superior não garante a meta de 100 Hz: um DATA de 261 bytes em 152.000 bit/s ocupa aproximadamente 17,17 ms no fio. A validação pré-Play e os ensaios devem rejeitar uma configuração cujo orçamento observado não caiba no passo, sem mudar `h` ou recuperar etapas.

RTS/CTS não será pesquisado nem usado neste incremento. São sinais físicos de controle de fluxo e não comprovam consumo pelo host; READ_ACK continua sendo a confirmação cumulativa requerida por F-27.

## Respostas Q incorporadas

| ID | Resposta recebida e efeito | Situação |
|---|---|---|
| Q-01 | Referência disponível em Projects/OT1-HiLInfrastructure; coordenador, RX/TX e shared state inspecionados; MID 04 reserva XRCE sob o mesmo dono do enlace | Resolvida para implementação incremental; custo/MTU dependem da versão escolhida |
| Q-02 | Consolidado com o serviço RaspDAQ: little-endian, SYNC 59 72; CONFIG 4/5 bytes, STATUS uint8 só no retorno CONFIG; DATA base com SEQ e N fixo por direção | Base definida em ADR-003; não repetir pergunta de ordem/tamanho/STATUS. Extensões têm status próprio |
| Q-03 | Manter checkbox. 100 passos consecutivos inválidos por canal adquirido → Error e novo Play, identificando todos os canais/input FMU que atingiram limite | Resolvida; substitui limite 101 e escopo de saída da revisão 0.3 |
| Q-04 | Retenção no host; sem histórico DAQC válido, usar valor inicial válido da entrada FMU. Ausência de start literal não impede inicialização calculada | Resolvida; não inferir zero nos atuadores ou no input sem referência válida |
| Q-05 | Recursos com reservas/exclusões atendem; ADC/PWM configuráveis conforme TARGET; mapa em volts; 0…255 é código DAC | Resolvida para configuração; exatidão/faixa útil exigem bancada |
| Q-06 | ≤5 ms por transferência; descartar atrasado; último snapshot antes da inserção; grade fixa e SEQ no fio; sem recuperar DATA | Resolvida |
| Q-07 | Continuar com aviso em falha de log; registrar saída FMU; identidade XML/nome/tipo/valueReference; NaN ou zero com qualidade por tipo | Resolvida em IF-LOG; validar vetores antes de congelar a versão |
| Q-08 | Identificar e informar capacidades não suportadas | Resolvida; não declarar suporte universal nem restringir silenciosamente |
| Q-09 | Em STREAMING, 60 s sem avanço de leitura → DISABLE, sem zeramento. READ_ACK MID 03 confirma o último SEQ consumido; dois núcleos ESP32 | Layout resolvido; cadência/tolerância são parâmetros medidos, sem sessão no fio |

## Esclarecimentos técnicos

**RaspDAQ:** o processo cria uma instância duradoura de SharedDaqState e a entrega ao runtime USB e nó rclpy. Os snapshots imutáveis são substituídos sob RLock; o nó não é recriado por snapshot. FunctionFS fornece o lado dispositivo USB em Linux. A placa ESP32 informada usa CH340/UART; o padrão de estado pode ser aproveitado, mas endpoints FunctionFS não são um driver ESP32. O protocolo RaspDAQ usa outros SYNC, comandos e comprimentos; não substitui o ICD MICROHIL. [Evidência de inspeção](evidence/q-review-2026-09-06.md).

**Volts e códigos:** 0…255 são os 256 códigos do DAC de 8 bits. A referência ideal é Vout ≈ código × VDD3P3_RTC / 255. O mapa analógico permanece em volts; a conversão para código é explícita e a tensão útil depende de caracterização. PWM é duty cycle/frequência; não é tensão analógica contínua sem circuito apropriado. [Fonte DAC](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/dac.html).

**IDs da FMU:** valueReference serve às chamadas FMI e não é necessariamente único entre todas as variáveis. Não significa posição sequencial do arquivo. O contrato propõe usar a ordem XML das saídas selecionadas e guardar índice, nome, tipo e valueReference associados à FMU. [Schema oficial](https://raw.githubusercontent.com/modelica/fmi-standard/v2.0.4/schema/fmi2ScalarVariable.xsd).

**SCHED_FIFO:** continua obrigatório. Configurar a política pode falhar, por exemplo por privilégios insuficientes (EPERM); a aplicação precisa verificar retorno e política efetiva. Proposta: impedir Play HiL e informar a causa se falhar, sem fallback silencioso. Isso não conflita com continuar após deadline perdido durante uma execução já iniciada. [Linux man-pages](https://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html).

**Constância e comunicação:** um valor constante válido não incrementa inválidos e sua leitura comprova progresso. A proteção de aquisição conta passos no host; DISABLE conta 60 s sem progresso de confirmação de leitura no firmware. Um dado NaN efetivamente lido avança a confirmação, mas participa da contagem de invalidade no host. Essa separação evita desabilitar a DAQC apenas porque a medição é constante ou numericamente inválida.

## Detalhes de implementação e validação

| ID | Contrato consolidado | Parâmetro/evidência restante |
|---|---|---|
| Q-01 | RaspDAQ tem rclpy/FunctionFS; MICROHIL acrescenta MID 04 XRCE sob coordenador único | Fixar micro-ROS/Agent, MTU XRCE 128, fragmentação, QoS e medir interferência |
| Q-02 | Consolidado com o serviço RaspDAQ: little-endian, SYNC 59 72; CONFIG 4/5 bytes, STATUS uint8 só no retorno CONFIG; DATA base com SEQ e N fixo por direção | Base definida em ADR-003; não repetir pergunta de ordem/tamanho/STATUS. Extensões têm status próprio |
| Q-05 | Configuração ADC/PWM segundo opções/limites do TARGET e validação antes de Play | Caracterizar circuito, faixa útil e combinações no alvo |
| Q-06 | Última atualização na inserção FMI; leitor próprio; ausência separada; próximo instante fixo; DATA perdido segue adiante | Medir scheduler, timeout e orçamento fim a fim |
| Q-07 | IF-LOG tipado/versionado; configuração binária separada | Implementar fixtures e vetores de round-trip/corrupção |
| Q-09 | READ_ACK 5 bytes com SEQ uint16; sem sessão/CRC/retransmissão; 60 s por relógio monotônico | Escolher cadência dentro do orçamento e medir tolerância sob saturação |

Respostas 1…5 e esclarecimento de grade fixa foram incorporados. Não reabrir seleção por passo, ausência separada, configuração ADC/PWM, zero físico no fim, restart pela FMU ou política temporal. Restam enumeração física do perfil, implementação integrada e critérios de ensaio; não são novas perguntas sobre escolhas já confirmadas.

## Parâmetros de enlace selecionados — revisão 0.11

O usuário selecionou UART **152.000 bit/s** como baseline e MTU XRCE **128 bytes**. São decisões de configuração, não evidência de operação. No fio, um frame XRCE completo tem 133 bytes (`SYNC`, `MID`, `LENGTH` e payload), ocupando aproximadamente 8,75 ms a 8N1. O escalonador TX deve preservar a prioridade de CONFIG, READ_ACK e DATA; XRCE continua best effort e pode ser descartado antes de atrasar tráfego crítico. A implementação TTY HOST aceita 152.000 bit/s por `termios2`/`BOTHER`, com verificação em pseudo-terminal; a taxa efetiva da ponte CH340 e da placa ainda precisa de ensaio físico.
## Correção de direção e uso dos núcleos

Mundo real → AI/DI DAQC → USB → host → input FMU é aquisição; output FMU → USB → AO/DO/PWM DAQC → mundo real é atuação. “Saída da DAQC” na GUI não deve designar ambiguamente o canal de aquisição com erro: informar canal físico adquirido e input FMU mapeado. A redação 0.3 que mandava zerar atuadores por ausência de host foi substituída, não mantida como política adicional.

Aprovado usar os dois núcleos ESP32 em benefício do timing. Proposta de distribuição: aquisição/atuação em um núcleo; comunicação, micro-ROS e supervisão no outro. Afinidades/IRQs/prioridades exatas serão dimensionadas e medidas; não alegar ausência absoluta de interferência por divisão de núcleos.

## Respostas 1…5 incorporadas — revisão 0.5

1. Sem novo pacote: reutilizar referência válida e registrar timeout separadamente; não incrementar invalidade numérica.
2. Vários pacotes: usar somente última atualização disponível antes da inserção na FMU. Leitor USB e simulação obrigatoriamente em threads distintas.
3. Atuação de dados somente STREAMING. Ao acabar a execução, zero físico e cessação de envio. Novo Play processa novamente inicialização da FMU e seus outputs iniciais. O zero do encerramento não é causado por uma amostra inválida isolada.
4. ADC e PWM configuráveis; opções e restrições explicitadas no TARGET a partir de fontes Espressif.
5. Confirmado pelo usuário: após cálculo 0→12 ms com passo 10 ms, próxima etapa começa em 20 ms, na grade fixa. Não saltar etapas da FMU, não executar rajadas e não alterar h. Mostrar perdas de deadline, instantes de execução perdidos e worst case após a simulação.

## Resultado da consulta RaspDAQ e fechamento 0.7

Base CONFIG/DATA, mapa posicional, sequência, ownership e workers RX/TX foram aproveitados. O MICROHIL acrescenta somente READ_ACK e XRCE para requisitos que a referência não atende. A decisão posterior remove sessão no fio, CRC e recuperação de DATA: o parser descarta o que não puder usar, conta gaps detectáveis e segue para o próximo. [ADR-003](adrs/ADR-003-raspdaq-icd.md) registra a decisão e seus limites.

## Observação de baseline micro-ROS — 0.8.1

Em 09.09.2026 foi feito clone limpo dos ramos `humble` e build HOST de `micro_ros_setup`, `micro_ros_msgs` e `micro_ros_agent` no Ubuntu 22.04.5 com ROS 2 Humble. Os commits observados foram `af209288676e5f02ac7c6d419b8ad157d3bed14e` (setup), `c9062eb3860d16c1bff1423923de3b0956fd4734` (mensagens), `c93ee764e0d2ef4907aeb29233c68cb5f4b56976` (Agent) e `57d086216d01ec43121845d385894a25987f8a2c` (Micro XRCE-DDS Agent baixado pelo build). O componente ESP-IDF no mesmo ramo foi obtido em `4ddd8c26e721662319ed8af981cb7cdc9ae05382`, mas ainda não foi compilado por não haver ESP-IDF instalado no host.

O README desse commit do componente declara testes para ESP-IDF 5.2, 5.3, 5.4, 5.5 e 6.0; não declara ESP-IDF 4.4.8. Em 09.09.2026, o usuário escolheu ESP-IDF v5.2.6, a menor série declarada, após confirmação de que o alvo `esp32` clássico é suportado. A evidência limitada está em [micro-ros-host-baseline-2026-09-09.md](evidence/micro-ros-host-baseline-2026-09-09.md); o build integrado do firmware e o ensaio na placa ainda estão pendentes.

## Interface ROS confirmada — 0.10.0

O usuário definiu os tópicos `/daqc_setup`, `/daqc_state`, `/daqc_errors` e `/daqc_data`. A interface detalhada está no [IF-ROS](contracts/interfaces.md#if-ros--tópicos-micro-ros-e-mensagens-por-perfil). Setup e state usam comando/estado `uint8` e identificador de perfil `uint32`; errors usa flags binárias. `/daqc_data` é telemetria não crítica por perfil e não substitui DATA MID 02.

Foi definida a mensagem `DaqcErrors` com flags binárias para ROS, comunicação, FMU, perfil, timeout e dado inválido, além de uma flag binária de origem. A telemetria ESP32 aguarda a enumeração aprovada dos I/O e nomes dos campos: não usar GPIOs ou capacidades possíveis como se fossem o perfil simultâneo. MTU XRCE de 128 bytes foi escolhido pelo usuário. Cliente, Agent customizado e buffers devem usar o mesmo valor; a enumeração física dos I/O ESP32 continua pendente.
