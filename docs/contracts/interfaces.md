# Contratos de interface

ICD-MICROHIL-001, revisão de trabalho 0.6. Base RaspDAQ inspecionada e adaptada por solicitação do usuário. **Origem de cada escolha:** R = observada na referência; U = decisão explícita MICROHIL; D = design derivado nesta revisão; E = extensão proposta porque não existe na referência. R/D não são evidência de execução; E não é funcionalidade já implementada ou decisão de produto aprovada. A base CONFIG/DATA abaixo está especificada; a liberação integrada depende das extensões identificadas, do transporte CH340 e de verificação. Não repetir perguntas já respondidas pelo código.

Registro de origem e diferenças: [ADR-003](../adrs/ADR-003-raspdaq-icd.md). Evidência estática: [EV-ICD-RASPDAQ](../evidence/raspdaq-icd-review-2026-09-06.md).

## IF-CORE — Importação, configuração e controle

Confirmado: FMU e configuração binária carregadas por ações separadas. Importação verifica FMI 2.0 Co-Simulation, nomes/tipos de inputs/outputs e compatibilidade de execução no host. Compatibilidade de formato não basta se a FMU só trouxer binário para outra arquitetura ou capacidade ainda não suportada; diagnosticar a causa específica. Q-08 autoriza identificar e informar capacidades não suportadas; a matriz do incremento explicitará String/Pending e demais capacidades.

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

Seleção confirmada: somente a última atualização recebida antes de inserir os inputs na FMU determina o candidato do passo. Thread USB independente publica candidato, qualidade, geração e último válido de forma coerente; a thread de simulação captura o snapshot no instante de inserção. Atualizações posteriores ficam para o próximo passo. No máximo um incremento de inválidos por canal/passo. Timeout sem atualização é contado separadamente: reter valor e não incrementar invalidade numérica; por design, manter o contador anterior até nova amostra válida (zera) ou inválida (incrementa), sem inventar dado válido. Pacote atrasado é descartado antes da seleção.

Proposta de encerramento: testar o contador antes de doStep; no instante do 100º passo inválido, entrar em Error sem executar essa etapa adicional. Registrar o passo da detecção separadamente dos passos concluídos. Este ponto exato de detecção ainda integra o detalhamento Q-06.

Outputs da FMU seguem o caminho de atuação. DATA é aplicado/transmitido somente durante STREAMING. Encerrar simulação implica zerar AO/DO/PWM e parar DATA; novo Play inicializa novamente a FMU e prepara seus outputs iniciais válidos. O protocolo deve garantir ordem e impedir DATA antigo após o zero. Falha de entrega não pode ser relatada como zero físico confirmado. Logging permanece de outputs FMU, não do input filtrado.

## IF-DAQ — Dono do enlace e adaptação do transporte

**R/D:** um serviço coordenador cria o estado compartilhado uma vez; leitura USB, escrita USB e ROS consomem/publicam snapshots. Só o coordenador gerencia dispositivo, workers e encerramento. Manter a camada C/libusb exigida no MICROHIL; Python é referência de design, não substituição obrigatória. O núcleo C não chama ROS nem aloca dataclasses em cada passo.

**U:** placa informada usa CH340/CH341, VID:PID 1a86:7523, conectando USB do host à UART do ESP32. Os VID/PID 1d6b:0104 e caminhos ep0/ep1/ep2 do RaspDAQ pertencem ao gadget Linux, não à placa atual. Descobrir endpoints Bulk nos descritores; não copiar endereços fixos. Exclusividade: não abrir TTY e libusb para consumir os mesmos bytes. Acesso direto via libusb deve configurar a ponte, verificar retorno das requisições, serial/framing e preservar a política de reset; detach/reattach do driver precisa ser controlado fora de STREAMING.

RaspDAQ não fornece inicialização CH340. A referência técnica complementar é o [driver CH341 Linux v6.8](https://raw.githubusercontent.com/torvalds/linux/v6.8/drivers/usb/serial/ch341.c), sem copiar código licenciado por inferência de compatibilidade. Baud rate e combinação de placa/clock/firmware serão medidas; 200 Hz no RaspDAQ não prova essa cadência no ESP32. Toda transferência MICROHIL permanece com timeout ≤5 ms; os 200/1000 ms do CLI de referência não são adotados.

## IF-WIRE-BASE — Formato derivado de RaspDAQ

Esta seção define a base posicional para vetores e reuso. O formato integrado com sessão/ACK/XRCE/CRC é uma extensão separada abaixo; não misturar receptores das duas revisões nem declarar que a base já fornece essas garantias.

| Elemento | MICROHIL definido nesta revisão | Origem |
|---|---|---|
| Inteiros multibyte e reais | Little-endian, sem padding nativo | R: struct com prefixo `<` |
| SYNC | uint16 0x7259 → bytes **59 72** | U: valor; R/D: ordem |
| MID | uint8 CONFIG=01, DATA=02 em ambos os sentidos | U; não copiar MID03 de retorno RaspDAQ |
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

Nenhum preenchimento até 251/256 bytes é necessário no MICROHIL. Na referência são 256 bytes totais, dos quais 251 de payload preenchido com zeros: não copiar esse teto para truncar os 256 bytes úteis aprovados. Campos ausentes não recebem zero silenciosamente, ao contrário do values.get(name,0.0) da referência.

### Vetores de layout base — não evidência de interoperabilidade

- ENABLE host: `59 72 01 02`; confirmação ENABLE: `59 72 01 02 02`.
- STREAMING host: `59 72 01 03`; confirmação: `59 72 01 03 03`.
- DISABLE host: `59 72 01 01`; confirmação: `59 72 01 01 01`.
- DATA de exemplo com SEQ=7 e dois float32 (1,0 V; 2,5 V): `59 72 02 07 00 00 00 80 3f 00 00 20 40`. É exemplo de schema sintético, não pinagem oficial.

## IF-MAP — Schema posicional e perfil

**R/D:** repetir o conceito FieldSpec/PayloadSchema: lista ordenada com identidade, tipo, unidade, largura e offset acumulado. Selecionar GPIOs disponíveis em ordem numérica crescente dentro de cada direção; uma função por GPIO. Um descritor de canal contém GPIO, função (AI/DI/AO/DO/PWM), tipo no fio, offset, largura, unidade, escala/offset da conversão e limites da configuração. ADC/PWM seguem TARGET. Unidade física de input FMU e valueReference ficam no mapa host; o MCU não interpreta a FMU.

Congelar schema antes de STREAMING. Payload recebido tem exatamente N bytes da direção; N=0 permite desativar a direção sem enviar DATA vazio periódico. Mudança de perfil exige parar e validar novamente. Não transportar nomes em cada amostra; nome/índice/tipo/valueReference ficam associados na configuração. Para compatibilidade, comparar versão de schema, perfil e hash do descritor canônico nos dois lados; negociação está em E-SESSION abaixo. O RaspDAQ usa schemas compilados, não fornece essa negociação dinâmica.

## IF-PARSER — Frames, sequência e invalidade

**R:** descartar sequência repetida e antiga. Delta=(new−last) mod 65536; 0 repetida, 1 esperada, 2…32767 salto para frente, 32768…65535 antiga/ambígua. Salto é lacuna de comunicação, não NaN. Primeiro pacote de sessão validada inicia a referência. Não confundir SEQ com índice do passo FMI: várias aquisições podem ocorrer em um passo.

**D:** parser acumulador por direção, porque UART não preserva fronteiras de read. Procurar SYNC, validar MID, calcular comprimento pelo schema e consumir quadro completo; reter fragmento e processar vários quadros por leitura. Não buscar SYNC dentro de payload como terminador. Limitar memória/trabalho por ativação. Um prazo de recepção parcial expirado invalida o fragmento, não alimenta input FMI. Instante local de read não prova idade de aquisição remota; seq sozinha não permite expirar por tempo físico.

**Limite da referência:** ela passa o resultado de um read direto ao decoder, não implementa esse acumulador; valida SYNC/MID/tamanho/SEQ, sem CRC. Isso não detecta toda alteração de bits que ainda produza valor finito. Sessão antiga após reinício de SEQ também não é eliminada apenas pela comparação modular. Não declarar ressincronização robusta, integridade ponta a ponta ou descarte temporal remoto resolvidos pela base; extensões abaixo cobrem o design necessário.

Invalidade numérica é por canal no host, separada de corrupção do quadro. NaN recebido em quadro íntegro não deve ser filtrado invisivelmente pelo firmware: o host precisa observar a qualidade para F-24. O snapshot mantém bruto/qualidade e último válido de forma coerente. Última atualização antes da inserção é a selecionada; ausência incrementa timeout separado, mantendo contador de inválidos até novo dado válido/inválido. Com checkbox, testar limite 100 antes de chamar o próximo doStep; entrar em Error e realizar encerramento coordenado.

## IF-STATES — Ordem de controle e encerramento

**R/D:** uma transação de controle ordena mudança de estado e resposta; confirmação de STREAMING precede o primeiro DATA. Um único escritor serializa TX. Revalidar estado imediatamente antes de emitir DATA para que um pacote preparado antes de Stop não saia após a confirmação. Não copiar mutex segurado durante write_full bloqueante: realizar ordenação por fila de controle e dono TX, com deadline limitado.

| Evento | Efeito MICROHIL |
|---|---|
| Inicialização do serviço/firmware | DISABLE, sem DATA; recepção CONFIG permanece ativa |
| ENABLE válido | Entra em ENABLE/IDLE, sem DATA; não reinicia a FMU por iniciativa do firmware |
| Play | Host valida perfil/schema, inicializa FMU e prepara outputs iniciais válidos; abre sessão, solicita STREAMING e só publica atuação após confirmação |
| Primeiro output da execução | Escrito após STREAMING a partir da FMU inicializada; não copiar cache de execução anterior |
| STREAMING repetido na mesma sessão | Confirmar estado sem reiniciar sequência, relógios ou outputs; adaptação de idempotência |
| Fim/Stop/Error do host | Congelar produção, invalidar DATA pendente, solicitar saída de streaming com zero físico e aguardar confirmação fora do núcleo |
| ENABLE recebido durante STREAMING | Adaptação de STOP_STREAM/ENABLE da referência: zero físico coordenado, parar DATA, limpar cache da sessão e confirmar ENABLE |
| DISABLE explícito | Zero físico coordenado quando houver execução ativa, parar DATA, fechar sessão e confirmar DISABLE; manter parser disponível |
| 60 s sem avanço de confirmação de leitura | Desativação local conforme F-27; não confundir esse evento com zeramento coordenado pedido pelo host |
| Reinício/reconexão | Nova sessão/configuração; nunca retomar automaticamente atuação ou cache antigo |

Não copiar o quarto comando STOP_STREAM de RaspDAQ: três COMMANDs MICROHIL já permitem mapear parada normal para ENABLE e fechamento para DISABLE. Estado reportado é o efetivo, não o comando simplesmente ecoado. Comando desconhecido é rejeitado sem mudar estado; diagnóstico precisa da extensão de resposta ou estado divergente, não de um valor oficial inventado dentro de STATUS.

A base curta CONFIG não correlaciona respostas tardias nem identifica sessão; repetição de ENABLE/STREAMING exige correlação da extensão para liberar atuação real. Limite de tentativas e prazo agregado de controle são parâmetros técnicos da adaptação a medir, distintos de timeout ≤5 ms por transferência. Nenhum worker mantém bloqueio ilimitado para esperar confirmação.

## IF-QUEUES — Filas, ownership e prioridades

**R:** um SharedDaqState permanece vivo, snapshots substituídos sob lock, caches de última revisão e workers RX/TX separados. **D:** host usa estruturas prealocadas; simulação é único escritor do contador por passo. USB/ROS não chamam FMI nem compartilham ponteiros mutáveis com widgets.

TX organiza três classes: (1) encerramento/controle e confirmações; (2) última atualização DATA por direção; (3) tráfego XRCE limitado. Encerramento tem entrada reservada e invalida DATA pendente da sessão. Uma atualização DATA ainda não enviada pode ser substituída pela mais recente, contando coalescência; nunca manter fila crescente de atuação antiga. Não cortar/intercalar bytes dentro de um quadro já iniciado. XRCE não pode ser descartado/coalescido como dados de processo; sinalizar falha de transporte ao middleware se fila não puder aceitar.

Filas possuem limites derivados do maior frame/MTU e número máximo de mensagens pendentes. Capacidade numérica definitiva depende do formato estendido e orçamento de memória identificado; o RaspDAQ não fornece esses limites. 60 s é prazo de desativação, não capacidade de armazenamento. RX/CONFIG deve progredir quando TX estiver saturado. Prioridades e divisão entre os dois núcleos ESP32 seguem ARCH-FW; ordem de software não prova WCET nem imunidade à interferência.

## IF-EXTENSIONS — O que não existe no RaspDAQ e como adaptar

Estas propostas respondem aos itens sem equivalente na referência. Não constituem descrição de código encontrado. São trabalho de design autorizado, sem promover novos bytes a decisões previamente dadas pelo usuário.

| Extensão | Design recomendado | Limite ainda a resolver na implementação/validação |
|---|---|---|
| E-SESSION | Negociar revisão do protocolo, perfil/hash do schema e identificador uint64 novo por execução; associar dados e comandos à sessão. Não aceitar DATA de sessão encerrada | Layout integrado e mensagem de negociação não existem na base; impedir wrap/ambiguidade de SEQ durante ausência longa |
| E-ACK | Mensagem de confirmação cumulativa com sessão + última sequência DATA efetivamente lida; emitir no worker de comunicação, coalescendo confirmações pendentes | Fixar código/layout/cadência e janela máxima de seq; não reutilizar heartbeat simples como prova de consumo |
| E-INTEGRITY | CRC do quadro completo, versão/length e parsing validado antes de alterar estado. CRC-32/ISO-HDLC é proposta para vetores futuros | Não adicionar CRC apenas na documentação e declarar a base interoperável; layout integrado deve versionar essa alteração |
| E-TIME | Identificar aquisição/etapa e validade no contrato; descartar callbacks host de transferências já expiradas. Para expiração remota, negociar relógio/idade ou janela de pedidos correlacionados | Um write que expira pode ter enviado bytes; não prometer cancelamento retroativo nem comparar relógios não sincronizados |
| E-XRCE | Um único dono libusb/UART demultiplexa canal lógico XRCE e CONFIG/DATA; adaptar callbacks no cliente micro-ROS e no agente host | Não existe cliente/agente XRCE no serviço RaspDAQ; tamanho/fragmentação/cotas de canal precisam do formato integrado |

SEQ uint16 de RaspDAQ usa meia faixa para comparação. Por exemplo, 60 s a 1000 frames/s excede 32768 e pode tornar ACK modular ambíguo. Portanto a base não basta para qualquer taxa solicitada: extensão deve usar contador maior ou negociar limite/janela antes de aceitar frequência. O 200 Hz da referência é configuração, não taxa MICROHIL confirmada.

### Proposta de integração micro-ROS mantendo a arquitetura pedida

Adaptar o serviço coordenador para possuir bridge USB C e agente XRCE incorporado em componente C++ não crítico. Cliente ESP32 usa callbacks de transporte que passam pelo mesmo multiplexador; nenhum segundo processo/agente abre a TTY. Snapshots de I/O fazem a ponte com a thread FMI. O nó ROS host publica metadados/estado e recebe comandos por interfaces do coordenador; o cliente micro-ROS publica/subscreve mensagens de I/O no ESP32. Definir um único escritor final por canal: modo HiL usa DATA do núcleo; callbacks ROS nunca competem diretamente pelo atuador.

RaspDAQ demonstra o padrão de processo único, não a API micro-ROS. O transporte customizado existe no [cliente e agente micro-ROS](https://github.com/micro-ROS/micro-ros.github.io/blob/master/_docs/tutorials/advanced/create_custom_transports/index.md); isso fundamenta a proposta, sem provar sua integração ou seu custo na placa. Antes de implementar, definir se o canal oferece mensagens completas com fragmentação limitada ou stream enquadrado; o decoder CONFIG/DATA atual não serve como decoder XRCE.

## IF-READ-PROGRESS — Regra confirmada de supervisão

Em STREAMING, 60 s sem avanço de confirmação de leitura dispara DISABLE local. A thread de comunicação confirma DATA completo lido mesmo se um canal contiver NaN. ACK duplicado/antigo/de outra sessão não renova tempo. Relógio monotônico começa em STREAMING e é renovado por progresso válido. Supervisão separada da tarefa crítica e sem busy loop; medir tolerância entre limiar e transição.

Progresso via confirmação pode deixar de chegar porque o caminho de retorno falhou; é detecção conservadora de consumo comprovado, não acesso ao buffer interno CH340. Não suspender watchdog esperando um TX bloqueado nem acumular backlog durante 60 s. E-ACK/E-SESSION completam o mecanismo; não afirmar que ele foi encontrado na referência.

## IF-LOG — Registro binário e configuração: design host, não código RaspDAQ

A referência inspecionada apresenta valores/snapshots no terminal; não foi encontrado nela o formato de log/configuração binário MICROHIL. Portanto as decisões abaixo são **D**, derivadas dos requisitos confirmados, não recuperação de um formato existente.

Definir arquivo versionado, little-endian, sem dump de struct. Cabeçalho proposto: magic ASCII `MHILLOG1` (8 bytes), versão uint16=1, tamanho do cabeçalho uint32, SHA-256 da FMU (32 bytes), t_start e h float64, número de saídas uint32. Após campos fixos, uma entrada por saída selecionada: índice XML uint32, valueReference uint32, código de tipo uint8, tamanho do nome UTF-8 uint16 e nome. Ordem canônica: índices XML crescentes, não ordenar por valueReference. Não presumir unicidade de valueReference. String e capacidades fora do incremento são diagnosticadas antes de iniciar.

Registro proposto: índice da etapa concluída uint64, tempo simulado float64, bitmap de qualidade com ceil(n/8) bytes (bit i=1 significa inválido), valores na ordem do cabeçalho. Real usa float64 preservando tipo FMI, Integer/Enumeration int32, Boolean uint8. Real inválido → NaN, tipos discretos inválidos → zero com bit de qualidade; valor finito válido da FMU permanece bruto mesmo se não puder ser convertido para atuação. Bits excedentes do bitmap são zero. Métricas por passo não são incluídas. Lacunas no índice revelam registros descartados, sem inventar amostras. Limitar contagens/tamanhos e verificar overflow antes de alocar.

CSV somente após encerramento; conferir hash/índice/nome/tipo/valueReference com a FMU, não carregar um modelo diferente só porque tem igual quantidade de saídas. Cabeçalho não exige executar a FMU para interpretar dados. Tipo do log não é o float32 do fio. Arquivo truncado exporta somente registros completos com aviso; nunca declarar log integral se houve falha de abertura/escrita/flush/close. Simulação continua com aviso quando logging falha.

Configuração: formato separado, magic `MHILCFG1`, versão uint16=1, tamanho total uint32 e seções TLV (tipo uint16, comprimento uint32, bytes). Seções obrigatórias: identidade FMU, perfil/schema, mapa ordenado, execução; opcionais: GUI/gráficos. Não permitir tipos obrigatórios desconhecidos ou duplicados. Validar arquivo inteiro contra FMU/DAQ antes de aplicar; definir os campos de cada seção com o schema de perfil integrado, sem serializar endereços/objetos Qt/Python. Este desenho resolve direção e portabilidade do formato; vetores completos de configuração dependem do schema final de E-SESSION.

## IF-OUTPUT-LIFECYCLE — Atuação e zero final

Somente STREAMING aplica DATA de atuação. Inicializar novamente a FMU a cada Play e transmitir seus outputs iniciais válidos após confirmação de STREAMING. Em fim/Stop/Error, parar produção, solicitar transição coordenada que zera saídas e descarta pendências antes de confirmar estado. F-23/F-24 são filtros de inputs no host; o zero físico é consequência do encerramento, não de uma amostra inválida isolada.

A ordem de locks/estado antes da confirmação em RaspDAQ fundamenta a transação; não copiar limpeza de cache como prova de zero elétrico. Confirmação de estado só é emitida após execução do procedimento de saída no firmware. Quando houver perda do enlace, informar falta de confirmação em vez de afirmar zeramento remoto. F-27 mantém a semântica separada de desativação local por 60 s sem leitura.

## IF-TIME — Execução sem compensação

Aguardar próxima liberação da grade fixa após overrun, sem alterar h ou saltar etapas FMI. Medir por separado etapas atrasadas, liberações não usadas e pior atraso, conforme ARCH-TIME; apresentar somente após a execução. Thread de leitura USB publica snapshots independentes, sem alterar a grade ou chamar a FMU. O snapshot consumido é o último publicado antes da inserção dos inputs.
