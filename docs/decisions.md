# Decisões e esclarecimentos da especificação

Revisão 0.7. Respostas DEC-001…012 e Q-01…09 incorporadas. **Implementação permanece suspensa somente até concluir e verificar esta consolidação dos Markdown.** A revisão vigente acrescenta a decisão de comunicação real-time: sem CRC, retransmissão ou recuperação de DATA; perda detectável é contabilizada e o fluxo segue para o pacote novo. READ_ACK e XRCE usam MIDs próprios. O histórico anterior permanece nas evidências.

## Decisões de produto

| ID | Confirmado | Detalhamento restante |
|---|---|---|
| DEC-001 | USB-C/CH340 e micro-ROS no ESP32; coordenador C único usa `/dev/ttyUSB*` e ponte UDP local do Agent; XRCE MID 04; tópicos `/daqc_setup`, `/daqc_state` e `/daqc_errors` | MTU XRCE 128 bytes selecionado; integrar Agent e medir custo |
| DEC-002 | Qt 6/C++ com Qt Quick/QML desacoplado, terminal debug, prioridade GUI inferior ao núcleo, aparência Mint; log binário tipado e CSV posterior | Validar UX e formatos por fixtures |
| DEC-003 | Perfil ESP32, recursos selecionáveis com exclusão por GPIO e reserva UART; ADC/DAC internos e PWM configurável | Caracterização elétrica continua requisito de bancada |
| DEC-004 | FMU 2.0 CS sem planta fixa, passo/duração configuráveis; meta 100 Hz medida por modelo/alvo; grade fixa, sem compensação; USB ≤5 ms por transferência | Medir orçamento fim a fim por modelo/perfil |
| DEC-005 | Aquisição inválida retida no host, inicial válido do input se necessário; checkbox/100 passos. Encerramento zera saídas físicas e cessa DATA; restart reinicializa FMU | Verificar ordem e confirmação em HIL |
| DEC-006 | Primeiro Linux Ubuntu 22.04.5/ROS 2 Humble; futuro Raspberry Pi 4/2 GB; firmware usa ESP-IDF v5.2.6 e ramos Humble de micro-ROS | Fixar e verificar a baseline integrada no ESP32; kernel/arquitetura do Pi seguem pendentes |
| DEC-007 | Continuar após overrun; último dado válido pode ser reutilizado até fim; saída constante não é defeito; SCHED_FIFO deve ser verificado | Falha de configuração impede Play HiL e gera diagnóstico |
| DEC-008 | Saída da FMU por passo no binário; Real inválido usa NaN, discreto inválido usa zero com bit de qualidade; falha de gravação gera aviso e não interrompe | Formato IF-LOG deve ser testado antes de estabilizar versão 1 |
| DEC-009 | Sem pausa; gráfico até 10 Hz, ticks Y configuráveis, janela temporal comum deslizante; fechar destrói histórico, reabrir começa dali; configuração e abertura separadas, abertura antes de Play permitida | Sem conflito pendente com proteção: ela termina em Error |
| DEC-010 | Preservar APIs Qt; convenção própria C++ e estilo definidos na constituição | Nenhuma decisão de toolkit reaberta |
| DEC-011 | Importação de FMU separada da configuração YAML; diagnosticar incompatibilidades específicas com FMU/DAQ/mapa | Pode identificar e informar capacidades não suportadas no incremento inicial (Q-08 resolvida) |
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

### Revisão 0.26.0 — aquisição DAQC configurável

O usuário confirmou que a aquisição deve permanecer ativa em tarefa separada durante STREAMING e ter frequência configurável. A taxa representa a leitura periódica de AI/DI, não a frequência de passos da FMU. A DAQC mantém somente o snapshot corrente; o host usa o último DATA completo recebido antes da fronteira FMI, sem criar fila crescente ou executar a FMU no leitor.

O YAML passa a aceitar `acquisition.frequency_hz`; perfis novos gravados pela GUI iniciam em 100 Hz. A implementação inicial admitia 1…400 Hz com base no orçamento nominal do frame ESP32 de 33 bytes a 152.000 bit/s; esse número não era medição da CH340 nem garantia temporal.

### Revisão 0.26.1 — cadência limitada pelo tick FreeRTOS

A inspeção de `sdkconfig` confirmou `CONFIG_FREERTOS_HZ=100`. A conversão de 1 ms para ticks retornava zero e mantinha a tarefa de I/O em polling. Para preservar o núcleo de I/O, a implementação passa a aguardar explicitamente um tick e admite `acquisition.frequency_hz` de 1…100 Hz. A mudança não altera o passo da FMU nem cria recuperação; expandir essa faixa exigirá uma decisão de agendamento de maior resolução e nova medição no ESP32.


| ID | Contrato consolidado | Parâmetro/evidência restante |
|---|---|---|
| Q-01 | RaspDAQ tem rclpy/FunctionFS; MICROHIL acrescenta MID 04 XRCE sob coordenador único | Fixar micro-ROS/Agent, MTU XRCE 128, fragmentação, QoS e medir interferência |
| Q-02 | Consolidado com o serviço RaspDAQ: little-endian, SYNC 59 72; CONFIG 4/5 bytes, STATUS uint8 só no retorno CONFIG; DATA base com SEQ e N fixo por direção | Base definida em ADR-003; não repetir pergunta de ordem/tamanho/STATUS. Extensões têm status próprio |
| Q-05 | Configuração ADC/PWM segundo opções/limites do TARGET e validação antes de Play | Caracterizar circuito, faixa útil e combinações no alvo |
| Q-06 | Última atualização na inserção FMI; leitor próprio; ausência separada; próximo instante fixo; DATA perdido segue adiante | Medir scheduler, timeout e orçamento fim a fim |
| Q-07 | IF-LOG tipado/versionado; configuração YAML separada | Implementar fixtures e vetores de round-trip/corrupção |
| Q-09 | READ_ACK 5 bytes com SEQ uint16; sem sessão/CRC/retransmissão; 60 s por relógio monotônico | Escolher cadência dentro do orçamento e medir tolerância sob saturação |

Respostas 1…5 e esclarecimento de grade fixa foram incorporados. Não reabrir seleção por passo, ausência separada, configuração ADC/PWM, zero físico no fim, restart pela FMU ou política temporal. A enumeração funcional do perfil ESP32 e `profile_id = 1` foram confirmados em 10.09.2026; restam implementação integrada e critérios de ensaio.

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

Base CONFIG/DATA, mapa posicional, sequência e ownership foram aproveitados. O MICROHIL implementa o ownership da TTY com um worker serial único que alterna RX e TX limitados, separado da simulação; isso preserva o processo único da referência sem introduzir dois leitores ou escritores concorrentes. O MICROHIL acrescenta somente READ_ACK e XRCE para requisitos que a referência não atende. A decisão posterior remove sessão no fio, CRC e recuperação de DATA: o parser descarta o que não puder usar, conta gaps detectáveis e segue para o próximo. [ADR-003](adrs/ADR-003-raspdaq-icd.md) registra a decisão e seus limites.

## Observação de baseline micro-ROS — 0.8.1

Em 09.09.2026 foi feito clone limpo dos ramos `humble` e build HOST de `micro_ros_setup`, `micro_ros_msgs` e `micro_ros_agent` no Ubuntu 22.04.5 com ROS 2 Humble. Os commits observados foram `af209288676e5f02ac7c6d419b8ad157d3bed14e` (setup), `c9062eb3860d16c1bff1423923de3b0956fd4734` (mensagens), `c93ee764e0d2ef4907aeb29233c68cb5f4b56976` (Agent) e `57d086216d01ec43121845d385894a25987f8a2c` (Micro XRCE-DDS Agent baixado pelo build). O componente ESP-IDF no mesmo ramo foi obtido em `4ddd8c26e721662319ed8af981cb7cdc9ae05382`, mas ainda não foi compilado por não haver ESP-IDF instalado no host.

O README desse commit do componente declara testes para ESP-IDF 5.2, 5.3, 5.4, 5.5 e 6.0; não declara ESP-IDF 4.4.8. Em 09.09.2026, o usuário escolheu ESP-IDF v5.2.6, a menor série declarada, após confirmação de que o alvo `esp32` clássico é suportado. A evidência limitada está em [micro-ros-host-baseline-2026-09-09.md](evidence/micro-ros-host-baseline-2026-09-09.md); o build integrado do firmware e o ensaio na placa ainda estão pendentes.

## Interface ROS confirmada — 0.13.0

O usuário definiu somente os tópicos `/daqc_setup`, `/daqc_state` e `/daqc_errors`. A interface detalhada está no [IF-ROS](contracts/interfaces.md#if-ros--tópicos-micro-ros-de-controle-e-diagnóstico). Setup e state usam comando/estado `uint8` e identificador de perfil `uint32`; errors usa flags binárias. O MID 04 permanece reservado a XRCE e não substitui DATA MID 02.

A mensagem `DaqcErrors` contém flags binárias para ROS, comunicação, FMU, perfil, timeout e dado inválido, além de uma flag binária de origem. MTU XRCE de 128 bytes foi escolhido pelo usuário. Cliente, ponte UDP e buffers devem usar o mesmo valor.

## Autoridade de estado DAQC — 0.18.0

Em 10.09.2026, o usuário confirmou que CONFIG é a única autoridade das transições DISABLE, ENABLE e STREAMING no enlace crítico. `DaqcSetup.command` deve coincidir com o estado já efetivo e confirma, pelo caminho ROS, a seleção do perfil e a configuração ADC/PWM. A DAQC rejeita uma divergência e publica o diagnóstico binário apropriado; não há arbitragem por ordem de chegada entre duas autoridades. Essa separação mantém ROS fora do caminho crítico de CONFIG/DATA e evita que dois comandos concorram diretamente pelas saídas.

## Ciclo Play/Stop e confirmação CONFIG — 0.19.0

Em 10.09.2026, o usuário definiu que Play envia a mudança ENABLE→STREAMING e só então inicia a simulação. Stop envia STREAMING→DISABLE. O prazo agregado para confirmar CONFIG é configurável pelo operador, com valor inicial de 10 ms; o limite de cada transferência permanece 5 ms. A espera ocorre fora da thread de simulação e uma confirmação ausente impede Play ou é reportada durante o encerramento, sem deslocar o relógio da FMU.

## Formato YAML de ADC/PWM — 0.20.0

Em 10.09.2026, o usuário confirmou o formato persistido da configuração ADC/PWM. A seção obrigatória `adc` contém `resolution_bits` e um mapa `attenuation` com os seis canais AI do perfil, na ordem lógica GPIO32/33/34/35/36/39. A seção obrigatória `pwm` contém os dois canais GPIO18 e GPIO19, cada um com `frequency_hz` e `resolution_bits`. A GUI deve gerar todos os campos, inclusive para canais que não tenham mapa FMU, sem valores implícitos. O host valida a forma e os limites representáveis; a confirmação final de um par PWM e de sua aplicação continua sendo `DaqcState.configuration_applied` emitido pela DAQC.

## Ponte local do Agent — 0.21.0

Em 10.09.2026, o usuário aprovou substituir a premissa de transporte customizado dentro do Agent Humble por uma ponte UDP local. A inspeção da fonte oficial `micro-ROS-Agent` Humble no commit `c93ee764e0d2ef4907aeb29233c68cb5f4b56976` mostrou suporte direto apenas a SerialPort e UDP. O modo serial não pode ser usado, pois abriria a CH340 em concorrência com o coordenador C. O coordenador continua sendo o único leitor/escritor da UART e converte somente frames MID 04 em datagramas UDP para um Agent em loopback; o fluxo de retorno UDP é reenquadrado como MID 04 pelo mesmo coordenador. CONFIG, DATA e READ_ACK não passam pelo UDP, continuam com prioridade própria e não são visíveis ao Agent. A porta UDP local é configurável pelo integrador; `8888` é apenas o valor inicial do serviço, sem significado no enlace físico.

## Nó ROS 2 no runner — 0.22.0

O preflight do runner C deve reproduzir o baseline físico validado: depois de abrir a TTY como dono exclusivo, descarta apenas RX pendente e exige uma confirmação CONFIG DISABLE nova antes de ENABLE. Isso ocorre fora da simulação e não altera DATA, XRCE, o relógio FMI ou o estado físico além do DISABLE seguro. A primeira execução física desse baseline não resolveu a ausência de resposta Agent→DAQC; a evidência limita o resultado.

O usuário confirmou que o próprio `fmu_rt_runner` será um nó ROS 2 em C, usando `rcl`. O componente ROS executa em thread de controle separada: publica `DaqcSetup`, recebe `DaqcState` e `DaqcErrors`, armazena confirmações e acorda somente o controlador de Play/Stop. Ele não chama FMI, não acessa a TTY/CH340 e não espera dentro da thread de simulação. O controlador prepara a FMU, solicita ENABLE por CONFIG, publica o setup correspondente e aguarda `profile_applied` e `configuration_applied`; só então solicita STREAMING e cria a thread de simulação. A prioridade, latência e comportamento sob carga exigem medição HOST/HIL.

`daqc_config_timeout_ms` inicia em 10 ms e rege a confirmação CONFIG no enlace crítico. Após CONFIG ENABLE e antes de publicar `DaqcSetup`, `daqc_ros_startup_delay_ms` inicia em **6.000 ms** e é configurável pelo operador. Ele é uma espera de estabilização do Agent/XRCE reproduzida do procedimento físico validado; não confirma configuração. O prazo separado `daqc_ros_timeout_ms` inicia em **100 ms** e limita a espera por `DaqcState` com perfil/configuração aplicados depois da publicação. Ambos ocorrem uma vez por Play, antes de STREAMING, e não compõem o orçamento de nenhum passo FMI.

## Perfil compilado e XRCE HOST — 0.14.0

`DaqcSetup.profile_id` seleciona um perfil compilado na DAQC. A configuração HOST associa a FMU ao perfil selecionado e valida a compatibilidade antes do Play. O mapa de GPIOs e descritores permanecem compilados, mas `DaqcSetup` transfere a configuração limitada de ADC/PWM aprovada para aplicação em DISABLE ou ENABLE. `DaqcState` confirma o identificador, o perfil e a configuração aplicados.

O coordenador HOST mantém uma única mensagem XRCE pendente de até 128 bytes. A mensagem mais nova substitui a anterior e é enviada somente depois de CONFIG, READ_ACK e DATA pendentes. Confirmações CONFIG limpam esse mailbox, impedindo transportar controle XRCE de uma transição anterior. Isso implementa o multiplexador local; a ponte UDP do Agent e o cliente micro-ROS continuam pendentes de integração completa.

## Mapa funcional ESP32 — 0.15.0

O usuário confirmou o perfil simultâneo ESP32: AI GPIO36/39/34/35/32/33; AO GPIO25/26; DI GPIO27/14/13/4; DO GPIO16/17/21/22/23; PWM GPIO18/19. GPIO1/3 são reservados à UART0 e GPIO2/5/12/15 ao boot. O mapa usa `float32` para AI/AO/PWM e campos binários para DI/DO, resultando em DATA DAQC→host de 28 bytes e host→DAQC de 21 bytes. A ordem canônica e offsets estão no IF-MAP. O valor confirmado de `uint32 profile_id` é 1.

## Configuração ADC/PWM pelo setup — 0.16.0

O usuário aprovou ampliar `DaqcSetup` sem criar tópicos adicionais. A mensagem transporta resolução ADC comum, atenuação por AI, frequência e resolução por PWM, além de `apply_configuration`. A DAQC aplica esses parâmetros somente em DISABLE ou ENABLE e confirma pelo novo campo binário `DaqcState.configuration_applied`. `DaqcErrors` passa a indicar separadamente erro de configuração ADC e PWM por flags binárias. Duty PWM continua no DATA crítico; não há telemetria ROS adicional.

## Tipos FMI no perfil físico ESP32 — 0.24.0

Em 11.09.2026, o usuário confirmou que `Integer` e `Enumeration` não possuem conversão física aprovada para o perfil ESP32. Eles ficam disponíveis somente para entradas virtuais e para logging/gráficos de saída. O mapeamento físico aceita exclusivamente `Real` nos canais AI/AO/PWM e `Boolean` nos canais DI/DO. A GUI não oferece canais DAQC para os dois tipos discretos e o carregador YAML rejeita esse vínculo antes de aplicar o perfil.

## Persistência de entradas virtuais — 0.25.0

Em 11.09.2026, o usuário confirmou que YAML é exclusivamente configuração e não transporta dados operacionais. Valores de entradas virtuais não são gravados nem restaurados por um perfil. A GUI envia o valor à sessão operacional; antes do Play ele é mantido para a inicialização e, durante Running, é entregue ao `InputState` para consumo na fronteira do próximo passo FMI. A GUI não chama a instância FMI diretamente. Uma entrada já mapeada ao perfil DAQC não aceita fonte virtual concorrente.


## Cliente XRCE e recuperação — 0.25.7

A chave inicial fixa do cliente micro-ROS é `0x4D48494C`; o integrador deve torná-la distinta por DAQC ativa quando houver mais de uma no mesmo Agent. O framing serial interno do Micro XRCE-DDS fica desabilitado, pois a UART já é enquadrada exclusivamente pelo MID 04 do ICD. A tentativa de iniciar ou recuperar o cliente ocorre uma vez por segundo somente em DISABLE ou ENABLE, nunca durante STREAMING. A política não adiciona tráfego de recuperação ao caminho crítico.

A gravação física de 11.09 confirmou CONFIG pela CH340. Um ensaio anterior observou tráfego XRCE cru e resposta do Agent; após a correção de framing e reordenação de inicialização, a sessão XRCE completa ainda não foi observada. Essa pendência não autoriza inferir compatibilidade de tópicos ou tempo real.

## Diagnóstico de inicialização ROS — 0.26.7

Fato observado no código que não respondeu CONFIG após a revisão 0.26.6: `app_main`, cuja pilha é configurada em 4096 bytes, chamava `DaqcRosStart` de forma síncrona depois de criar a tarefa UART. A hipótese é que a inicialização micro-ROS nessa pilha possa resetar ou falhar antes de a recepção CONFIG permanecer disponível; ela ainda não foi confirmada em placa. A correção diagnóstica cria a supervisora de ROS com 8192 bytes no núcleo 0 após a tarefa UART e delega a ela toda chamada de início. A supervisora continua tentando somente em DISABLE ou ENABLE, uma vez por segundo, e não altera o ICD, os MIDs nem a prioridade de CONFIG.

O ensaio posterior compilou e gravou 0.26.7 com hashes confirmados. Depois de reset, espera de 1 s e cinco tentativas CONFIG DISABLE em intervalo de 200 ms, recebeu cinco confirmações completas; sem reset, recebeu nove em dez tentativas. MID 04 de inicialização apareceu na janela. Portanto, a tentativa única do harness após 200 ms era insuficiente e não sustenta bloqueio de inicialização. O harness aguarda após reset e limita tentativas, intervalo e timeout total; a confirmação CONFIG não valida XRCE, Agent, tópicos ou timing.

O procedimento manual comprovado abre a CH340 antes de ajustar DTR/RTS; o harness foi alinhado a essa ordem e usa timeout de leitura de 20 ms. Em uma ponte temporária de 8 s, com a DAQC em DISABLE e Agent Humble UDP local na porta 8888, foram observados 468 frames MID 04 da DAQC para o Agent e 285 no sentido Agent→DAQC. `ros2 node list` mostrou `/microhil_daqc`, e `ros2 topic list` mostrou `/daqc_errors`, `/daqc_setup`, `/daqc_state`, `/parameter_events` e `/rosout`. Isso comprova a sessão observada nesse estado, sem autorizar STREAMING, DATA, I/O, timing ou HIL.

Foi identificado também que o leitor de confirmação do harness recebia chunks da UART, mas não os anexava ao buffer do parser. A correção anexa o chunk antes da extração de frames e é coberta para resposta CONFIG normal e fragmentada. Esse defeito explica os falsos negativos do harness; não é evidência de defeito da UART, do firmware ou do ICD.
