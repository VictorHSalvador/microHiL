# Contratos de interface

ICD-MICROHIL-001, revisão de trabalho 0.2. Decisões recebidas aplicadas; **ICD ainda não liberado para implementação**. Pendências Q-01…09 em [decisions.md](../decisions.md). Nenhum campo sugerido foi inserido no protocolo como aprovação implícita.

## IF-CORE — Importação, configuração e controle

Confirmado: FMU e configuração binária carregadas por ações separadas. Importação verifica FMI 2.0 Co-Simulation, nomes/tipos de inputs/outputs e compatibilidade de execução no host. Compatibilidade de formato não basta se a FMU só trouxer binário para outra arquitetura ou capacidade ainda não suportada; diagnosticar a causa específica. Q-08 trata String/Pending sem excluir silenciosamente parte da meta genérica.

Configuração é comparada à FMU atual e ao perfil DAQC ESP32: variável, tipo, direção, função/pino, escala/unidade e parâmetros de execução. Listar divergências antes de aplicar, sem atualização parcial. Um GPIO não pode cumprir funções simultâneas incompatíveis. Step/duração são configuráveis na GUI/terminal e validados antes de iniciar.

Controle interno: Play, Stop, atualização de entradas virtuais e checkbox de proteção de inválidos. Sem pausa manual; ação automática do checkbox ainda Q-03. Snapshots/comandos são validados e aplicados na fronteira de ciclo; GUI não modifica configuração compartilhada sem contrato.

## IF-SAMPLE — Valores e último aceitável

A seleção de saída deve manter tipo/identidade da FMU. Último valor aceitável é referência para Stop, fim, erro, NaN/Inf e demais inválidos. Não fazer cast silencioso de físico para código ADC/DAC, nem substituir valor inválido por zero por conveniência. Rejeição por faixa deve ocorrer na representação/unidade do mapa.

Separar: output bruto, valor aceito para envio e valor cuja aplicação foi confirmada. Em link perdido não há como garantir nova entrega; tentar enviar “último válido” não comprova que ele chegou. Q-07 define qual valor vai para log; Q-09 define semântica de confirmação/retenção sob desconexão.

Proteção opcional dispara após mais de 100 inválidos consecutivos (101º). Contagem por canal/global e retomada aguardam Q-03. Valor válido pode zerar o contador como proposta, não decisão silenciosa. Se ainda não existe nenhum valor válido, a regra de retenção não tem candidato; Q-04 define esse caso e start ausente/calculado.

Proposta interna: snapshots completos, sequência/geração, relógio monotônico de recepção e qualidade; writer publica sob lock curto e leitor copia. Não deduzir idade apenas de número de frame nem comparar relógios host/MCU sem sincronização. Identidade/sessão no fio ainda Q-09.

## IF-DAQ — Topologia e propriedade do enlace

Confirmado: Bulk/libusb com arquitetura específica e micro-ROS no ESP32; usar USB-C da placa atual. O relato de hardware identifica CH340; caminho host USB → CH340 → UART do MCU. A [Espressif](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/establish-serial-connection.html) explica essa separação. USB-C não identifica um protocolo USB nativo do firmware.

Ainda necessário: acesso/control requests à ponte, configuração da UART, exclusividade frente ao driver, callbacks/transport do agente e relação com frames I/O. Python para leitura foi hipótese; NF-06 C/libusb não foi revogado. Um único dono deve controlar o stream; não planejar agente TTY e leitor libusb independentes concorrendo pelos mesmos bytes.

Um nó ROS com snapshots de I/O não implementa sozinho o protocolo do cliente micro-ROS. Transporte customizado exige considerar cliente e agente XRCE; a API oficial permite tal extensão, mas não torna CONFIG/DATA um protocolo XRCE automaticamente. Fonte: [micro-ROS custom transports](https://github.com/micro-ROS/micro-ros.github.io/blob/master/_docs/tutorials/advanced/create_custom_transports/index.md). Q-01 decide segundo transporte ou multiplexação, após conferir RaspDAQ.

## Campos e formatos confirmados

| Campo | Definido | Aberto |
|---|---|---|
| SYNC | 2 bytes, valor fixo 0x7259 | Ordem 72 59 ou 59 72 deve ser explícita, Q-02 |
| MID | 1 byte; CONFIG=0x01, DATA/PAYLOAD=0x02 | Nenhum novo MID adicionado para XRCE sem decisão |
| COMMAND | 1 byte; DISABLE=0x01, ENABLE=0x02, STREAMING=0x03 | Confirmação, repetição e idempotência detalhadas em Q-02/Q-09 |
| PAYLOAD | Até 256 bytes, uso conforme I/Os; não é obrigatório utilizar todos | Tamanho fixo por perfil/direção versus LENGTH, offsets, encoding e byte order |
| STATUS | Somente enviado ESP32→host; deve informar estado | “Repetição de MID” conflita com três estados. Tamanho, códigos e presença em DATA aguardam Q-02 |
| Real no protocolo | IEEE 754 float32 conforme NF-20 | Unidades/faixas/conversão de valores FMU e byte order |

Formato geral informado: `SYNC | MID | COMMAND/PAYLOAD | STATUS`, com STATUS apenas no retorno. Tradução por direção, deixando ambiguidades visíveis:

| Direção/modo | Formato de trabalho |
|---|---|
| Host→ESP32 CONFIG | SYNC[2] + MID=01[1] + COMMAND[1] |
| ESP32→host CONFIG | SYNC[2] + MID=01[1] + COMMAND[1] + STATUS[tamanho pendente] |
| Host→ESP32 DATA | SYNC[2] + MID=02[1] + PAYLOAD[N], 0≤N≤256 como limite estrutural; mínimo válido depende do perfil |
| ESP32→host DATA | SYNC[2] + MID=02[1] + PAYLOAD[N] + STATUS? (confirmar) |

**Mudança de NF-25:** 256 bytes limitam o payload. Um DATA com 256 bytes já tem 259 bytes com SYNC/MID, antes de STATUS ou outro cabeçalho que venha a ser aprovado. Não impor o limite antigo de 256 ao frame inteiro.

Proposta de STATUS para confirmação: um byte com 01 DISABLE, 02 ENABLE, 03 STREAMING, repetindo os códigos de estado/COMMAND. Não marcar essa proposta como bytes oficiais até Q-02. MID=01/02 indica tipo de mensagem e não distingue três estados.

## Framing e parser em streaming

Parser deve acumular bytes de leituras parciais e consumir vários frames por leitura. Identificar SYNC/MID, determinar comprimento conforme contrato e só então encaminhar CONFIG ou DATA. Receber 256 bytes em um read não define fronteira. Não procurar próximo SYNC dentro do payload como terminador: esses bytes podem fazer parte de um float ou dado digital.

A falta de tamanho explícito é solucionável por comprimento fixo conhecido de perfil/direção; alternativamente há campo LENGTH, mas ele não foi autorizado ainda. Também faltam integridade, ressincronização após perda de bytes, identificação de sessão e perfil, Q-02/Q-09. SYNC não detecta toda corrupção nem indica qual firmware está no outro lado.

## Estados da DAQC

| Estado | Comportamento confirmado | Detalhe pendente |
|---|---|---|
| DISABLE | Estado desligado de streaming; deve continuar apto a receber controles | Condição elétrica, aquisição interna e valor antes do primeiro modelo |
| ENABLE (IDLE) | DAQC habilitada aguardando streaming | Aplicação de valores iniciais e comandos DATA fora de STREAMING |
| STREAMING | Troca de dados e atendimento de CONFIG contínuos | Taxa de aquisição/publicação e carga máxima |

DISABLE recebido durante STREAMING deve interromper o streaming e permitir liberação do enlace, sem travar o parser ou exigir novo boot. Não desligar a recepção de CONFIG ao entrar em streaming. A confirmação deve representar estado realmente assumido, não mero eco anterior à transição. Prazo e ACK exatos aguardam Q-09.

Fechamento normal pode enviar DISABLE e aguardar confirmação limitada (proposta). Encerramento forçado, cabo removido ou processo morto pode impedir esse comando. Não confundir retenção enquanto MCU segue alimentado com restauração após reset. Watchdog/inatividade não recebem timeout novo por inferência dos 5 ms de USB.

## Timing e layout de I/O

Teto confirmado de timeout: 5 ms; Q-06 define se por operação ou troca completa. O timeout da API e o tempo real de conclusão sob scheduler são medições diferentes. Aquisição deve entregar amostra coerente e sua idade precisa de limite separado. Taxa mais rápida possível será a maior taxa estável validada no conjunto host/ponte/UART/firmware, não um número inventado.

Layout por perfil deve definir direção, quantidade, função por GPIO, unidade, offset, tamanho, escala, faixa e valores inválidos. ADC/DAC internos não requerem que todo valor do modelo use código cru; mapa precisa converter. Ordem de bool/integer/real, padding e endianness são contrato, não layout nativo de struct C/Python.

## IF-LOG — Saídas binárias e conversão

Confirmado: uma amostra final por saída selecionada por passo entregue; não registrar todos os subpassos da FMU. Gravar binário durante execução; converter a CSV apenas após encerramento, usando FMU para interpretar tipos. Não manter gravação CSV simultânea como formato principal. Métricas por passo não entram nesse stream; estatísticas finais são mantidas para GUI.

FMU fornece nomes/tipos, mas sozinha não diz quais variáveis foram selecionadas, em que ordem foram gravadas e a qual execução o arquivo pertence. Proposta de cabeçalho: versão de formato, identidade/hash da FMU, lista ordenada das saídas e parâmetros temporais. Definir timestamps/índices para distinguir tempo simulado e atraso real; não inferir que o último passo é inteiro. Q-07 confirma metadados e valores brutos/aceitos.

FMI Real e float32 USB não devem ser tratados como formatos idênticos; serialização do arquivo terá larguras/byte order explícitos e tratamento de String caso suportada. Não usar dump de memória de uma struct como formato portátil. Carregar FMU incompatível para conversão deve produzir erro em vez de interpretar bytes com tipo errado.

Consumidor assíncrono deve observar fim da produção, drenar itens finais e verificar escrita/flush/close. O fechamento da GUI de gráficos não fecha o logger. Disco/fila cheia nunca autoriza sucesso silencioso; decisão entre continuar incompleto ou encerrar em Error permanece Q-07. Não prometer recuperação de toda amostra após queda abrupta de energia sem contrato específico.
