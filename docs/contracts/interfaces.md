# Contratos de interface

ICD-MICROHIL-001, revisão de trabalho 0.7. Base RaspDAQ inspecionada e adaptada por solicitação do usuário. **Origem de cada escolha:** R = observada na referência; U = decisão explícita MICROHIL; D = design derivado necessário para integrar requisitos confirmados. R/D não são evidência de execução. A política vigente elimina CRC, retransmissão e recuperação de DATA; pacote perdido não bloqueia o fluxo seguinte.

Registro de origem e diferenças: [ADR-003](../adrs/ADR-003-raspdaq-icd.md). Evidência estática: [EV-ICD-RASPDAQ](../evidence/raspdaq-icd-review-2026-09-06.md).

## IF-CORE — Importação, configuração e controle

Confirmado: FMU e configuração YAML carregadas por ações separadas. Importação verifica FMI 2.0 Co-Simulation, nomes/tipos de inputs/outputs e compatibilidade de execução no host. Compatibilidade de formato não basta se a FMU só trouxer binário para outra arquitetura ou capacidade ainda não suportada; diagnosticar a causa específica. Q-08 autoriza identificar e informar capacidades não suportadas; a matriz do incremento explicitará String/Pending e demais capacidades.

Configuração é comparada à FMU atual e ao perfil DAQC ESP32: variável, tipo, direção, função/pino, escala/unidade e parâmetros de execução. Listar divergências antes de aplicar, sem atualização parcial. Um GPIO não pode cumprir funções simultâneas incompatíveis. Step/duração são configuráveis na GUI/terminal e validados antes de iniciar.

Controle interno: Play, Stop e atualização de entradas virtuais. A proteção F-24 termina em Error ao atingir 100 passos inválidos consecutivos por canal DAQC/input FMU; checkbox mantido por confirmação do usuário. Snapshots/comandos são aplicados na fronteira do ciclo; GUI não altera diretamente estado da FMU.

## IF-SAMPLE — Aquisição, retenção e contagem no host

## Direção dos sinais — terminologia normativa

| Caminho | Significado |
|---|---|
| Mundo real → entradas físicas DAQC (AI/DI) → USB → host → inputs FMU | Aquisição. O dado transmitido pela DAQC é uma entrada física adquirida e alimenta uma entrada do modelo |
| Outputs FMU → host → USB → saídas físicas DAQC (AO/DO/PWM) → mundo real | Atuação. Saída física da DAQC sempre significa sinal entregue ao mundo real |

A retenção de F-23 e a contagem de F-24 pertencem à aquisição no host. A mensagem identifica canal DAQC e input FMU. Amostra inválida isolada não manda zero aos atuadores; quando a simulação termina (fim, Stop ou Error), as saídas físicas são zeradas e o envio de dados é encerrado. Novo Play reinicializa a FMU e aplica os outputs iniciais válidos, sem reutilizar outputs da execução anterior.

O adaptador USB do host mantém o último dado válido por canal adquirido, separado do dado bruto e de sua qualidade. NaN/Inf/invalidade de tipo ou faixa não sobrescreve esse histórico. A thread de simulação recebe um snapshot coerente e usa o valor retido no input FMU. Não converter inválido em zero nem enviar zeramento aos atuadores. Primeiro dado inválido sem histórico: usar o valor inicial válido do input FMU, conforme Q-04 confirmado. Não o rotular como aquisição DAQC válida. Se essa referência também faltar/for inválida, não inventar zero; validar inicialização antes de doStep dependente.

Contagem por passo: um canal pode incrementar no máximo uma vez por passo da simulação. Com checkbox habilitado, no 100º passo consecutivo inválido, a proteção termina em Error, identificando todos os canais que atingiram o limite e seus inputs FMU. Válido constante é válido e quebra a sequência. Um run novo reinicia contadores. Checkbox desligado mantém o filtro e a retenção, sem término pela contagem. A invalidade não é contada por pacote USB, por leitura do parser ou por subpasso FMI.

Seleção confirmada: somente a última atualização recebida antes de inserir os inputs na FMU determina o candidato do passo. Thread USB independente publica candidato, qualidade, geração e último válido de forma coerente; a thread de simulação captura o snapshot no instante de inserção. Atualizações posteriores ficam para o próximo passo. No máximo um incremento de inválidos por canal/passo. Timeout sem atualização é contado separadamente: reter valor e não incrementar invalidade numérica; por design, manter o contador anterior até nova amostra válida (zera) ou inválida (incrementa), sem inventar dado válido. Pacote atrasado, repetido, antigo, incompleto ou perdido não é recuperado: descartar, contabilizar quando detectável e seguir para o próximo.

Proposta de encerramento: testar o contador antes de doStep; no instante do 100º passo inválido, entrar em Error sem executar essa etapa adicional. Registrar o passo da detecção separadamente dos passos concluídos. Este ponto exato de detecção ainda integra o detalhamento Q-06.

Outputs da FMU seguem o caminho de atuação. DATA é aplicado/transmitido somente durante STREAMING. Encerrar simulação implica zerar AO/DO/PWM e parar DATA; novo Play inicializa novamente a FMU e prepara seus outputs iniciais válidos. O protocolo deve garantir ordem e impedir DATA antigo após o zero. Falha de entrega não pode ser relatada como zero físico confirmado. Logging permanece de outputs FMU, não do input filtrado.

## IF-DAQ — Dono do enlace e adaptação do transporte

**R/D:** um serviço coordenador C cria o estado compartilhado uma vez; leitura, escrita e ROS consomem/publicam snapshots. Só o coordenador gerencia `/dev/ttyUSB*`, workers e encerramento. O núcleo C não chama ROS nem aloca dataclasses em cada passo.

**U:** placa informada usa CH340/CH341, VID:PID 1a86:7523, conectando USB do host à UART do ESP32. Os VID/PID 1d6b:0104 e caminhos ep0/ep1/ep2 do RaspDAQ pertencem ao gadget Linux, não à placa atual. O coordenador abre um único `/dev/ttyUSB*`, configura 8N1/baud aprovado e preserva a política de reset fora de STREAMING. Não abrir um segundo processo, agente serial ou cliente libusb para consumir os mesmos bytes.

RaspDAQ não fornece inicialização CH340. A referência técnica complementar é o [driver CH341 Linux v6.8](https://raw.githubusercontent.com/torvalds/linux/v6.8/drivers/usb/serial/ch341.c), sem copiar código licenciado por inferência de compatibilidade. UART usa 8N1, RTS/CTS desabilitado e baud rate configurável de 9.600 a 152.000 bit/s, aplicado antes de STREAMING nos dois extremos; a baseline selecionada é 152.000 bit/s. Medir a combinação de payload, placa, clock e firmware; 200 Hz no RaspDAQ não prova essa cadência no ESP32. Toda transferência MICROHIL permanece com timeout ≤5 ms; os 200/1000 ms do CLI de referência não são adotados.

## IF-WIRE-BASE — Formato derivado de RaspDAQ

Esta seção define o formato posicional vigente. CONFIG e DATA preservam ao máximo a base RaspDAQ. READ_ACK e XRCE são MIDs adicionais necessários para F-27 e micro-ROS; nenhum quadro usa CRC.

| Elemento | MICROHIL definido nesta revisão | Origem |
|---|---|---|
| Inteiros multibyte e reais | Little-endian, sem padding nativo | R: struct com prefixo `<` |
| SYNC | uint16 0x7259 → bytes **59 72** | U: valor; R/D: ordem |
| MID | uint8 CONFIG=01, DATA=02; READ_ACK=03; XRCE=04 | U: CONFIG/DATA; D: READ_ACK/XRCE para requisitos já confirmados |
| COMMAND | uint8 DISABLE=01, ENABLE=02, STREAMING=03 | U; não copiar códigos 00/01/02/03 da referência |
| STATUS | uint8, códigos COMMAND do estado efetivo; somente resposta CONFIG ESP32→host | U: códigos; R/D: posição/tamanho. DATA não carrega STATUS |
| SEQ | uint16 por direção DATA | R/D; 65535→0 com comparação modular |
| PAYLOAD | N bytes fixos para perfil/direção durante a execução, N≤256 | R: layout fixo; U: teto útil 256; D: transmitir apenas bytes utilizados |
| Campos analógicos/PWM | IEEE 754 float32 em volts/duty normalizado | U/D: NF-20 e TARGET |
| Digital | uint8, 0/1 válido; outro código inválido | D; sem bitfields/padding implícito |
| Integer/Enumeration quando aplicável | int32 em complemento de dois; domínio validado pelo contrato da variável | D; não fazer cast silencioso |

A diferença entre escolha anterior sugerida 72 59 e a ordem agora 59 72 decorre da instrução de usar RaspDAQ para fechar o assunto; a escolha anterior era proposta, não confirmação de bytes. O valor numérico 0x7259 permanece.

| Direção | Quadro base | Tamanho |
|---|---|---|
| Host→DAQ CONFIG | SYNC[2] MID[1] COMMAND[1] | 4 |
| DAQ→host CONFIG | SYNC[2] MID[1] COMMAND[1] STATUS[1] | 5 |
| DATA, qualquer direção | SYNC[2] MID=02[1] SEQ[2] PAYLOAD[N] | 5+N, máximo 261 |
| Host→DAQ READ_ACK | SYNC[2] MID=03[1] SEQ[2] | 5 |
| XRCE, qualquer direção | SYNC[2] MID=04[1] LENGTH[2] PAYLOAD[L] | 5+L; L limitado pelo MTU configurado |

Nenhum preenchimento até 251/256 bytes é necessário no MICROHIL. Na referência são 256 bytes totais, dos quais 251 de payload preenchido com zeros: não copiar esse teto para truncar os 256 bytes úteis aprovados. Campos ausentes não recebem zero silenciosamente. LENGTH existe apenas no quadro XRCE, porque DATA tem N fixo por direção.

### Vetores de layout base — não evidência de interoperabilidade

READ_ACK para SEQ 7: 59 72 03 07 00.

- ENABLE host: `59 72 01 02`; confirmação ENABLE: `59 72 01 02 02`.
- STREAMING host: `59 72 01 03`; confirmação: `59 72 01 03 03`.
- DISABLE host: `59 72 01 01`; confirmação: `59 72 01 01 01`.
- DATA de exemplo com SEQ=7 e dois float32 (1,0 V; 2,5 V): `59 72 02 07 00 00 00 80 3f 00 00 20 40`. É exemplo de schema sintético, não pinagem oficial.

## IF-MAP — Schema posicional e perfil

**R/D:** repetir o conceito FieldSpec/PayloadSchema: lista ordenada com identidade, tipo, unidade, largura e offset acumulado. Selecionar GPIOs disponíveis em ordem numérica crescente dentro de cada direção; uma função por GPIO. Um descritor de canal contém GPIO, função (AI/DI/AO/DO/PWM), tipo no fio, offset, largura, unidade, escala/offset da conversão e limites da configuração. ADC/PWM seguem TARGET. Unidade física de input FMU e valueReference ficam no mapa host; o MCU não interpreta a FMU.

Congelar schema antes de STREAMING. Payload recebido tem exatamente N bytes da direção; N=0 permite desativar a direção sem enviar DATA vazio periódico. Mudança de perfil exige parar e validar novamente. Não transportar nomes em cada amostra; nome, índice, tipo e valueReference ficam associados na configuração. Host e firmware devem possuir a mesma versão de perfil/schema antes de habilitar STREAMING.

O perfil ESP32 confirmado usa os campos abaixo em ordem de GPIO crescente. DAQC→host tem 28 bytes: quatro DI e seis AI. Host→DAQC tem 21 bytes: cinco DO, dois PWM e dois AO. Todos os `float32` seguem IEEE 754 little-endian; campos binários só aceitam 0 ou 1.

| Direção | Offset | Campo genérico | Tipo |
|---|---:|---|---|
| DAQC→host | 0 | `GPIO4_DI` | `uint8` |
| DAQC→host | 1 | `GPIO13_DI` | `uint8` |
| DAQC→host | 2 | `GPIO14_DI` | `uint8` |
| DAQC→host | 3 | `GPIO27_DI` | `uint8` |
| DAQC→host | 4 | `GPIO32_AI` | `float32` |
| DAQC→host | 8 | `GPIO33_AI` | `float32` |
| DAQC→host | 12 | `GPIO34_AI` | `float32` |
| DAQC→host | 16 | `GPIO35_AI` | `float32` |
| DAQC→host | 20 | `GPIO36_AI` | `float32` |
| DAQC→host | 24 | `GPIO39_AI` | `float32` |
| Host→DAQC | 0 | `GPIO16_DO` | `uint8` |
| Host→DAQC | 1 | `GPIO17_DO` | `uint8` |
| Host→DAQC | 2 | `GPIO18_PWM` | `float32` |
| Host→DAQC | 6 | `GPIO19_PWM` | `float32` |
| Host→DAQC | 10 | `GPIO21_DO` | `uint8` |
| Host→DAQC | 11 | `GPIO22_DO` | `uint8` |
| Host→DAQC | 12 | `GPIO23_DO` | `uint8` |
| Host→DAQC | 13 | `GPIO25_AO` | `float32` |
| Host→DAQC | 17 | `GPIO26_AO` | `float32` |

CONFIG 01/02/03 altera somente o estado da DAQC. A seleção do perfil usa `DaqcSetup.profile_id` pelo MID 04 em DISABLE ou ENABLE, nunca dentro do ciclo. O identificador seleciona um perfil compilado no firmware; `DaqcState.profile_id` e `profile_applied` confirmam sua aplicação. O arquivo de configuração HOST contém o mapeamento entre FMU e o perfil selecionado e deve ser validado integralmente antes do Play. Esta revisão não transporta descritores, mapa de GPIOs ou parâmetros ADC/PWM variáveis para a DAQC pelo enlace. STREAMING só é aceito quando o perfil selecionado e o schema do host são compatíveis. O perfil ESP32 usa `profile_id = 1` e layout fixo de 28 bytes de aquisição e 21 bytes de atuação. O RaspDAQ usa schemas compilados e não fornece essa seleção micro-ROS.

## IF-PARSER — Frames, sequência e invalidade

**R/D:** descartar sequência repetida e antiga. Delta=(new−last) mod 65536; 0 repetida, 1 esperada, 2…32767 salto para frente, 32768…65535 antiga/ambígua. Salto é lacuna de comunicação: contabilizar os pacotes ausentes e aceitar imediatamente o pacote novo, sem NACK ou retransmissão. O primeiro DATA após a transição para STREAMING inicia a referência. Não confundir SEQ com índice do passo FMI: várias aquisições podem ocorrer em um passo.

**D:** parser acumulador por direção, porque UART não preserva fronteiras de read. Procurar SYNC, validar MID, determinar comprimento pelo tipo/schema e consumir quadro completo; reter fragmento por prazo limitado e processar vários quadros por leitura. Não buscar SYNC dentro de payload como terminador de um quadro cujo comprimento já é conhecido. Um prazo parcial expirado descarta o fragmento e reinicia a busca; não alimenta input FMI. Limitar memória e trabalho por ativação.

**Limite aceito:** o protocolo não usa CRC. SYNC, MID, tamanho, estado e SEQ detectam erros estruturais, mas não toda alteração de bits que continue sintaticamente válida. A validação por tipo, finitude e faixa pode rejeitar valores de canal inválidos; valor corrompido ainda plausível pode passar. Registrar essa limitação na evidência e não declarar integridade ponta a ponta.

Invalidade numérica é por canal no host, separada de corrupção do quadro. NaN recebido em quadro íntegro não deve ser filtrado invisivelmente pelo firmware: o host precisa observar a qualidade para F-24. O snapshot mantém bruto/qualidade e último válido de forma coerente. Última atualização antes da inserção é a selecionada; ausência incrementa timeout separado, mantendo contador de inválidos até novo dado válido/inválido. Com checkbox, testar limite 100 antes de chamar o próximo doStep; entrar em Error e realizar encerramento coordenado.

## IF-STATES — Ordem de controle e encerramento

**R/D:** uma transação de controle ordena mudança de estado e resposta; confirmação de STREAMING precede o primeiro DATA. Um único escritor serializa TX. Revalidar estado imediatamente antes de emitir DATA para que um pacote preparado antes de Stop não saia após a confirmação. Não copiar mutex segurado durante write_full bloqueante: realizar ordenação por fila de controle e dono TX, com deadline limitado.

| Evento | Efeito MICROHIL |
|---|---|
| Inicialização do serviço/firmware | DISABLE, sem DATA; recepção CONFIG permanece ativa |
| ENABLE válido | Entra em ENABLE/IDLE, sem DATA; não reinicia a FMU por iniciativa do firmware |
| Play | Host valida perfil/schema, limpa mailboxes/sequências, inicializa FMU e prepara outputs iniciais válidos; solicita STREAMING e só publica atuação após confirmação |
| Primeiro output da execução | Escrito após STREAMING a partir da FMU inicializada; não copiar cache de execução anterior |
| STREAMING repetido sem saída intermediária do estado | Confirmar estado sem reiniciar sequência, relógios ou outputs; comando idempotente |
| Fim/Stop/Error do host | Congelar produção, invalidar DATA pendente, solicitar saída de streaming com zero físico e aguardar confirmação fora do núcleo |
| ENABLE recebido durante STREAMING | Adaptação de STOP_STREAM/ENABLE da referência: zero físico coordenado, parar DATA, limpar mailboxes e confirmar ENABLE |
| DISABLE explícito | Zero físico coordenado quando houver execução ativa, parar DATA, limpar mailboxes e confirmar DISABLE; manter parser disponível |
| 60 s sem avanço de confirmação de leitura | Desativação local conforme F-27; não confundir esse evento com zeramento coordenado pedido pelo host |
| Reinício/reconexão | DISABLE, nova validação de configuração e sequências reiniciadas; nunca retomar automaticamente atuação ou cache antigo |

Não copiar o quarto comando STOP_STREAM de RaspDAQ: três COMMANDs MICROHIL já permitem mapear parada normal para ENABLE e fechamento para DISABLE. Estado reportado é o efetivo, não o comando simplesmente ecoado. Comando desconhecido é rejeitado sem mudar estado; diagnóstico precisa da extensão de resposta ou estado divergente, não de um valor oficial inventado dentro de STATUS.

CONFIG é idempotente e confirmado pelo estado efetivo. Antes de mudar para STREAMING, o dono do enlace descarta fragmentos e DATA pendentes e redefine as sequências; DATA fora de STREAMING é rejeitado. O host pode repetir CONFIG dentro de um prazo agregado limitado, sem retransmitir DATA. Limite de tentativas e prazo agregado são parâmetros a medir, distintos do timeout de no máximo 5 ms por transferência. Nenhum worker mantém bloqueio ilimitado esperando confirmação.

## IF-QUEUES — Filas, ownership e prioridades

**R:** um SharedDaqState permanece vivo, snapshots substituídos sob lock e caches de última revisão. **D:** o host usa um worker serial único como dono da TTY, separado da thread de simulação; ele alterna RX e TX limitados, preservando a prioridade emitida pelo coordenador. Simulação é único escritor do contador por passo. USB/ROS não chamam FMI nem compartilham ponteiros mutáveis com widgets.

TX organiza três classes: (1) encerramento/CONFIG e READ_ACK; (2) última atualização DATA por direção; (3) tráfego XRCE limitado. Encerramento tem entrada reservada e invalida DATA pendente da execução. Uma atualização DATA ainda não enviada pode ser substituída pela mais recente, contando coalescência; nunca manter fila crescente de atuação antiga. Não cortar/intercalar bytes dentro de um quadro já iniciado. READ_ACK também é coalescido para a maior sequência lida dentro do STREAMING atual. XRCE usa best effort; saturação é reportada ao middleware e não bloqueia CONFIG ou DATA.

DATA usa mailbox de capacidade um: produtor substitui valor ainda não consumido e incrementa contador de coalescência. Não há backlog, replay ou retransmissão. Filas de CONFIG/XRCE são limitadas pelo maior frame/MTU e por orçamento de memória a medir. Os 60 s são prazo de desativação, nunca capacidade de armazenamento. RX/CONFIG deve progredir quando TX estiver saturado. Prioridades e divisão entre os dois núcleos ESP32 seguem ARCH-FW; ordem de software não prova WCET nem imunidade à interferência.

## IF-EXTENSIONS — Adaptações mínimas ao RaspDAQ

| Adaptação | Contrato vigente | Limite a validar |
|---|---|---|
| READ_ACK, MID 03 | Host confirma cumulativamente o último SEQ DAQC→host efetivamente lido; quadro de 5 bytes, sem CRC, sem retransmissão e coalescível | Cadência deve impedir falso DISABLE sem gerar carga relevante; medir wrap e saturação |
| XRCE, MID 04 | Um único dono TTY/UART demultiplexa XRCE de CONFIG/DATA e entrega ou recebe payload pelo transporte customizado do Agent; LENGTH uint16 delimita a mensagem; mailbox HOST de 128 bytes coalesce mensagens pendentes; caminho best effort | Fixar fragmentação, executor e QoS na versão micro-ROS escolhida |
| Limpeza de execução | Transição para STREAMING limpa fragmentos/mailboxes e reinicia SEQ/ACK; DATA fora de STREAMING é descartado | Ensaiar CONFIG repetido, reconexão e bytes tardios |

Não existe identificador de sessão nem extensão de integridade no contrato vigente. O SEQ uint16 é usado somente no fluxo corrente e comparado dentro da meia faixa. O watchdog de 60 s mede tempo monotônico sem avanço de READ_ACK; não converte 60 s de pacotes em distância modular e não armazena histórico para recuperar perdas.

### Proposta de integração micro-ROS mantendo a arquitetura pedida

Adaptar o serviço coordenador para possuir bridge USB C e agente XRCE incorporado em componente C++ não crítico. Cliente ESP32 usa callbacks de transporte que passam pelo MID 04; nenhum segundo processo/agente abre a TTY. Snapshots de I/O fazem a ponte com a thread FMI. O nó ROS host recebe setup e publica state/errors pelas interfaces do coordenador; o cliente micro-ROS recebe setup e publica state/errors. Definir um único escritor final por canal: modo HiL usa DATA do núcleo; callbacks ROS nunca competem diretamente pelo atuador.

RaspDAQ demonstra o padrão de processo único, não a API micro-ROS. O transporte customizado existe no [cliente e agente micro-ROS](https://github.com/micro-ROS/micro-ros.github.io/blob/master/_docs/tutorials/advanced/create_custom_transports/index.md); isso fundamenta o MID 04, sem provar sua integração ou seu custo na placa. O multiplexador entrega uma mensagem XRCE completa delimitada por LENGTH; fragmentação acima do MTU fica a cargo do adaptador definido para a versão escolhida.

## IF-READ-PROGRESS — Regra confirmada de supervisão

Em STREAMING, 60 s sem avanço de READ_ACK dispara DISABLE local. A thread de comunicação confirma DATA completo lido mesmo se um canal contiver NaN. ACK duplicado, antigo ou fora do STREAMING atual não renova tempo. Relógio monotônico começa após o primeiro DATA transmitido e é renovado somente quando o SEQ confirmado avança. Supervisão separada da tarefa crítica e sem busy loop; medir tolerância entre limiar e transição.

Progresso via confirmação pode deixar de chegar porque o caminho de retorno falhou; é detecção conservadora de consumo comprovado, não acesso ao buffer interno CH340. Não suspender watchdog esperando TX bloqueado nem acumular backlog durante 60 s. READ_ACK não solicita reenvio e não foi encontrado na referência.

## IF-ROS — Tópicos micro-ROS de controle e diagnóstico

O MID 04 transporta somente XRCE-DDS entre o cliente micro-ROS da DAQC e o Agent integrado ao coordenador host. Ele não substitui DATA MID 02 para aquisição e atuação real-time. Os tópicos abaixo são a interface ROS 2/micro-ROS; sua publicação, serialização e tratamento ocorrem fora do caminho crítico da FMU.

| Tópico | Direção | Tipo | Finalidade |
|---|---|---|---|
| `/daqc_setup` | Host → DAQC | `microhil_interfaces/DaqcSetup` | Confirma o estado já solicitado por CONFIG, seleciona perfil e aplica configuração ADC/PWM fora de STREAMING |
| `/daqc_state` | DAQC → Host | `microhil_interfaces/DaqcState` | Confirma estado efetivo, perfil e configuração aplicados |
| `/daqc_errors` | Host e DAQC → consumidores | `microhil_interfaces/DaqcErrors` | Publica flags de falha; `source_is_daqc` identifica a origem |

`DaqcSetup.msg` contém `uint8 command`, `uint32 profile_id`, `uint8 apply_configuration`, `uint8 adc_resolution_bits`, `uint8[6] adc_attenuation`, `uint32[2] pwm_frequency_hz` e `uint8[2] pwm_resolution_bits`. `command` usa DISABLE=1, ENABLE=2 e STREAMING=3, coerente com CONFIG do ICD. CONFIG é a única autoridade para transições de estado no enlace crítico. A DAQC rejeita um `DaqcSetup.command` diferente do estado efetivo e usa o valor coincidente como confirmação ROS da configuração solicitada. `profile_id=1` identifica o perfil ESP32 compilado, com schema congelado e nomes de I/O definidos antes de STREAMING; não transporta mapa de pinos ou descritores variáveis no ciclo.

`apply_configuration` vale 0 ou 1. Com valor 1, a DAQC só aceita a configuração em DISABLE ou ENABLE; deve validá-la e aplicá-la fora do caminho crítico antes de confirmar STREAMING. `adc_attenuation` usa a ordem GPIO32/33/34/35/36/39 e os códigos 0 dB=0, 2,5 dB=1, 6 dB=2 e 11 dB=3. `pwm_frequency_hz` e `pwm_resolution_bits` usam a ordem GPIO18/19. Duty PWM é `float32` no DATA de atuação e não é parte deste setup.

`DaqcState.msg` contém `uint8 state`, `uint32 profile_id`, `uint8 profile_applied` e `uint8 configuration_applied`. `state` usa os mesmos códigos de COMMAND; `profile_applied` vale 1 somente quando o perfil indicado foi validado e aplicado. `configuration_applied` vale 1 somente quando a última configuração solicitada foi validada e aplicada. Isso é a confirmação ROS complementar, não substitui a confirmação CONFIG de cinco bytes no enlace.

As interfaces de controle estão materializadas no pacote `ros2/microhil_interfaces`; a build anterior no Humble está delimitada pela [evidência](../evidence/host-ros-interfaces-2026-09-09.md), anterior a esta extensão. `DaqcErrors.msg` contém somente flags `uint8`: `source_is_daqc`, `ros_error`, `communication_error`, `fmu_error`, `profile_error`, `adc_configuration_error`, `pwm_configuration_error`, `timeout_error` e `invalid_data_error`. Cada flag vale 0 ou 1. A DAQC publica `fmu_error=0`, pois não executa FMI; o host pode publicar essa flag quando a execução da FMU falhar. A GUI associa a origem e as flags ao diagnóstico estruturado local, sem depender de texto ou prints no firmware.


O MTU XRCE selecionado é 128 bytes. Cliente, transporte customizado no Agent, buffers e testes devem usar o mesmo valor. A fragmentação XRCE é permitida acima desse limite, mas não cria prioridade sobre CONFIG, READ_ACK ou DATA. Um frame XRCE iniciado não é intercalado; por isso, em 152.000 bit/s, o pior quadro de 133 bytes ocupa cerca de 8,75 ms no fio e sua emissão permanece best effort, sujeita ao orçamento medido do STREAMING.

## IF-LOG — Registro binário e configuração: contrato HOST, não código RaspDAQ

A referência inspecionada apresenta valores/snapshots no terminal; não foi encontrado nela o formato de log/configuração binário MICROHIL. Portanto as decisões abaixo são **D**, derivadas dos requisitos confirmados, não recuperação de um formato existente.

No HOST, o arquivo versionado implementado é little-endian e não faz dump de struct. O cabeçalho é magic ASCII `MHILLOG1` (8 bytes), versão uint16=1, tamanho do cabeçalho uint32, SHA-256 dos bytes da FMU (32 bytes), `t_start` e `h` float64 e número de saídas uint32. Após os campos fixos, há uma entrada por saída selecionada: índice XML uint32, valueReference uint32, código de tipo uint8, tamanho do nome UTF-8 uint16 e nome. O índice XML é one-based: a posição da `ScalarVariable` na lista do XML. A ordem canônica é índice XML crescente, não valueReference; não se presume unicidade de valueReference. String e capacidades fora do incremento são diagnosticadas antes de iniciar.

O registro implementado contém sequência da etapa concluída uint64, tempo simulado float64, bitmap de qualidade com ceil(n/8) bytes (bit i=1 significa inválido) e valores na ordem do cabeçalho. Real usa float64, Integer/Enumeration int32 e Boolean uint8. Real inválido é normalizado para NaN; discreto inválido, para zero com bit de qualidade. Bits excedentes do bitmap são zero. Métricas por passo não entram no formato. Lacunas na sequência revelam descartes, sem inventar amostras. Contagens, nomes, overflow e entradas truncadas/inconsistentes são limitados e diagnosticados antes da interpretação.

O logger SPSC assíncrono usa capacidade fixa de 128 posições; o produtor não executa I/O do sink e o consumidor é o único proprietário do arquivo. A capacidade é parâmetro pendente de medição, não limite validado para produto. Abertura, cabeçalho, registro, flush e close retornam a primeira falha estruturada; perdas e falhas deixam o log incompleto, sem interromper a simulação no mecanismo HOST.

CSV é ação explícita somente após encerramento. O conversor compara hash, índice XML, nome, tipo, valueReference e ordem com o descritor reconstruído da FMU atualmente carregada; `t_start` e `h` permanecem no formato, mas não participam da identidade FMU/esquema. As colunas são `sequence`, `simulation_time_s` e, por saída, `<nome>_value`, `<nome>_valid`. Não é necessário executar a FMU para interpretar o cabeçalho. Tipo do log não é o float32 do fio. Cauda truncada exporta somente registros completos com resultado parcial; arquivo incompleto por falha de logging não é disponibilizado como log íntegro para conversão. A GUI ainda deve definir apresentação e ação de produto.

Configuração usa YAML separado, versionado e com seções `version`, `fmu`, `profile`, `execution`, `adc`, `pwm`, `inputs` e `outputs`; GUI/gráficos são opcionais. Cada mapa usa o nome da variável FMU como identidade, seu tipo esperado e o canal genérico do perfil. O carregador resolve o `valueReference` na FMU carregada e pode comparar o valor registrado apenas como diagnóstico; ele não é a identidade portátil do arquivo. Não permitir seções obrigatórias desconhecidas ou duplicadas. Validar o arquivo inteiro contra FMU/DAQ antes de aplicar e não serializar endereços ou objetos Qt/Python. Os vetores ESP32 já têm canais, offsets, tamanhos e `profile_id = 1` definidos; os testes de interoperabilidade permanecem pendentes. O carregador HOST atual materializa o subconjunto `version`, `profile.id`, `execution` e a sequência única `mappings`, resolve direção/nome/tipo com a FMU e constrói os schemas DAQC→host de 28 bytes e host→DAQC de 21 bytes. No modo terminal, o YAML só é carregado após a FMU e, quando válido, fornece `step_size_s` e `stop_time_s`; Play exige que cada saída mapeada esteja selecionada para leitura. O codificador de atuação aplica a transformação inversa `raw=(valor_fmu-offset)/scale`; na primeira saída inválida sem histórico ele falha, e após um valor válido retém o último valor codificável. Um índice interno da lista de outputs da FMU, associado ao nome, tipo e `valueReference` resolvidos, preserva essa identidade depois da ordenação de apresentação/log. `DaqActuationZero` cria o payload de encerramento. A bridge HOST aceita esse payload no mailbox DATA apenas em STREAMING confirmado, mas o runner ainda não a chama. As seções YAML completas e a ligação desses schemas ao ciclo ainda são incrementos pendentes e não devem ser anunciadas como disponíveis.

## IF-OUTPUT-LIFECYCLE — Atuação e zero final

Somente STREAMING aplica DATA de atuação. Inicializar novamente a FMU a cada Play e transmitir seus outputs iniciais válidos após confirmação de STREAMING. Em fim/Stop/Error, parar produção, solicitar transição coordenada que zera saídas e descarta pendências antes de confirmar estado. F-23/F-24 são filtros de inputs no host; o zero físico é consequência do encerramento, não de uma amostra inválida isolada.

A ordem de locks/estado antes da confirmação em RaspDAQ fundamenta a transação; não copiar limpeza de cache como prova de zero elétrico. Confirmação de estado só é emitida após execução do procedimento de saída no firmware. Quando houver perda do enlace, informar falta de confirmação em vez de afirmar zeramento remoto. F-27 mantém a semântica separada de desativação local por 60 s sem leitura.

## IF-TIME — Execução sem compensação

Aguardar próxima liberação da grade fixa após overrun, sem alterar h ou saltar etapas FMI. Medir por separado etapas atrasadas, liberações não usadas e pior atraso, conforme ARCH-TIME; apresentar somente após a execução. Thread de leitura USB publica snapshots independentes, sem alterar a grade ou chamar a FMU. O snapshot consumido é o último publicado antes da inserção dos inputs.
