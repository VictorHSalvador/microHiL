# Alvo físico e ambiente

Status: perfil inicial denominado **ESP32**, com ADC/DAC internos, PWM e disponibilização dos I/Os disponíveis confirmados pelo usuário; mapa simultâneo/faixas de aquisição ainda em detalhamento. Fonte local: [ESP32-CONTROLADOR.md](references/ESP32-CONTROLADOR.md). As alegações de fotografia/esptool são informações recebidas; foto, comando completo, versão e saída bruta não foram inspecionados nesta sessão. Nenhuma nova consulta ao dispositivo foi executada.

## Inventário de origem

| Campo | Informação recebida | Limite/pendência |
|---|---|---|
| Placa | Formato DevKit V1, 30 pinos, USB-C | Fabricante, revisão e esquema desconhecidos |
| Identificador provisório | DAQC-PROT-ESP32-DEVKIT-30P-USB-C | Identifica protótipo, não perfil aprovado de produto |
| MCU | ESP32-D0WDQ5, revisão 3, informado por esptool | Preservar saída bruta antes de consolidar nomenclatura comercial/ECO |
| CPU/cristal | Dual-core, capacidade até 240 MHz; cristal informado 40 MHz | Clock configurado de firmware ainda não definido |
| USB–UART | CH340/CH341, VID:PID 1a86:7523; UART 8N1 configurável de 9.600 a 152.000 bit/s; baseline selecionada: 152.000 bit/s; RTS/CTS desabilitado | Confirmar variante, driver, reset e exclusividade do dispositivo; medir capacidade por perfil/passo |
| Porta | /dev/ttyUSB0 na identificação fornecida | Não é identificador persistente nem prova de presença atual |
| Flash | 4 MB informados | Partições, modo e frequência pendentes |
| ADC | Referência VRef em eFuse informada | Não comprova calibração de sistema ou exatidão da DAQC |
| Módulo | Blindagem/antena PCB no relato | Modelo comercial e conexões adicionais pendentes |
| Host produto | Raspberry Pi 4 com 2 GB, confirmado pelo usuário | Arquitetura do SO, kernel e recursos reservados pendentes; etapa inicial em máquina Linux Ubuntu 22.04/ROS 2 Humble |
| Host auditado | Ubuntu 22.04.5, x86-64, kernel 6.8.0-138-generic | Não representa o alvo Raspberry Pi |
| Software alvo | ROS 2 Humble, FMILibrary, Qt 6; ESP-IDF v5.2.6 e ramos Humble de micro-ROS | ESP32 clássico é alvo listado pelo ESP-IDF 5.2 e pelo componente Humble; build integrado e placa continuam pendentes |

## Evidência do host de build

Em 07.09.2026, a TASK-001 foi executada em Ubuntu 22.04 com GCC 11.4.0 e CMake disponível, no diretório `/home/linuxvh/Projects/microHiL`; cada build usou um diretório temporário limpo em `/tmp`. A FMILibrary oficial usada para o runner foi a 3.0.4, na revisão fixa `4a4b21ec10a632b2768a604c2330c54204919644`. O registro completo, inclusive os comandos e limitações, está em [host-build-foundation-2026-09-07.md](evidence/host-build-foundation-2026-09-07.md).

Esse host de build não representa nem qualifica o Raspberry Pi 4. A instalação externa exercitada em `/home/linuxvh/Projects/asturian-software/dev/external/include` e `/home/linuxvh/Projects/asturian-software/release/external/lib/libfmilib.a` comprovou somente a rota explícita de configuração sem download: sua versão não foi comprovada e não é baseline do produto.

## Recursos e reservas para planejamento

O documento recebido lista 25 GPIOs expostos: 21 capazes de saída e quatro somente entrada. Isso não autoriza 25 canais livres simultâneos. A tabela abaixo preserva capacidades e restrições, sem atribuir nomes de canais do produto.

| Grupo/pinos | Uso informado e limitação | Situação do projeto |
|---|---|---|
| GPIO34/35/36/39 | Somente entrada; candidatos ADC1 | Não atribuir saída; condicionamento/faixa pendentes |
| GPIO32/33 | ADC1 e digital bidirecional | Alocação pendente |
| GPIO25/26 | DAC interno de 8 bits ou outras funções multiplexadas | DAC interno confirmado; respeitar função exclusiva e definir unidade/escala de aplicação |
| GPIO1/3 | UART0 associada à ponte USB–UART | Reservar para arquitetura serial/programação; não misturar logs e protocolo sem contrato |
| GPIO2/5/12/15 | Strapping exposto | Não comprometer níveis de reset; alocação condicionada ao circuito |
| GPIO0 | Boot no relato, sem exposição nos headers | Não planejar canal no header |
| GPIO6–11 | Flash; não expostos no relato | Excluídos do perfil |
| GPIO16/17 | UART alternativa sugerida | Confirmar módulo/conexões antes de alocar |
| GPIO21/22, 18/19/23/5 | Sugestões I2C/SPI da fonte | Não são pinagem obrigatória; GPIO5 requer avaliação de boot |
| ADC2 | Nove canais expostos no relato, com compartilhamento de recursos | Avaliar uso de rádio/SDK; não assumir todos os canais disponíveis simultaneamente |

Sinais externos de processo, precisão ADC, tensão útil DAC e estado das saídas em reset dependem de circuito e ensaio. A faixa de alimentação do chip não define faixa aceita em VIN ou em GPIO. Limites absolutos não são condições normais de trabalho. Capacidades nominais de UART/I2C/PWM do chip não definem taxa fim a fim pela ponte ou pelo middleware.

## Topologia de comunicação

Para a placa descrita, o caminho é host USB → ponte CH340 → UART do ESP32. Não assumir USB CDC nativo de ESP32-S3. O coordenador C abre exclusivamente o dispositivo Linux `/dev/ttyUSB*` e demultiplexa CONFIG, DATA, READ_ACK e XRCE conforme o ICD. MID 04 alimenta o transporte customizado do Micro-ROS Agent; nenhum agente serial concorrente abre a porta.

A Espressif documenta explicitamente a divisão USB–ponte–UART. O componente micro-ROS no ramo Humble documenta transporte UART customizado e agente serial. O setup e Agent Humble tiveram build HOST limpo em 09.09.2026. O usuário selecionou ESP-IDF v5.2.6, da menor série declarada como testada pelo componente Humble atual; ESP32 clássico é alvo listado tanto pelo SDK quanto pelo componente. Isso comprova compatibilidade declarada de versão/alvo, não o build integrado nem o funcionamento na placa. Fontes: [ESP-IDF conexão serial](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/get-started/linux-macos-setup.html), [ESP-IDF ESP32 v5.2](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/), [micro_ros_setup Humble](https://github.com/micro-ROS/micro_ros_setup/tree/humble), [componente micro-ROS Humble](https://github.com/micro-ROS/micro_ros_espidf_component/tree/humble).

## Dados necessários para liberar o perfil

O perfil ESP32 expõe recursos configuráveis pelo usuário, respeitando exclusões por GPIO, reserva UART e compartilhamento de periféricos. ADC/PWM têm opções abaixo; SPI/I2C não receberam aplicações específicas.

| Grupo de capacidade | Recurso recebido | Faixa/representação a documentar |
|---|---|---|
| Digital | 25 GPIOs expostos; 21 capazes de saída, 4 somente entrada | Booleano 0/1; domínio nominal 3,3 V, não tolerância a 5 V. Limiares elétricos seguem chip/circuito, não uma faixa analógica livre |
| ADC1 | GPIO32/33/34/35/36/39 | Conversor nominal 12 bits; atenuação/resolução selecionáveis conforme opções abaixo; faixa calibrada identificada |
| ADC2 | GPIO2/4/12/13/14/15/25/26/27 | Mesma decisão de faixa; conflitos de rádio/boot/função devem ser respeitados |
| DAC | GPIO25/26, interno 8 bits confirmado | Código 0…255 corresponde nominalmente a 0…VDD3P3_RTC; tensão útil sob carga precisa ser caracterizada |
| UART0 | GPIO1/3 | Recurso usado pela ponte CH340; não disponível como GPIO livre enquanto esse enlace estiver ativo |
| PWM | Inclusão confirmada em Q-05; somente pinos capazes de saída e sem conflito | Frequência, resolução, duty e timers configuráveis conforme limites abaixo |
| SPI/I2C e demais funções | Capacidades multiplexadas da placa | Aplicações específicas não definidas; não alocar automaticamente |

Não existe uma faixa única “padrão 0…3,3 V” com a mesma precisão para todo ADC ESP32. Como referência técnica, a documentação ESP-IDF 4.4.4 recomenda 150…2450 mV para maior precisão com atenuação 11 dB; isso não fixa a atenuação deste firmware. O perfil deverá informar atenuação, calibração e unidade real transportada. Fontes primárias: [ADC](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/adc.html), [DAC](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/dac.html). Versão documental consultada, não SDK selecionado.

Para cada função habilitada: identidade, GPIO/conector, tipo/unidade, faixa e conversão, incompatibilidades, estado físico por modo e restrições, ciclo e procedimento de ensaio. Códigos crus ADC/DAC não equivalem automaticamente a valores da planta em volts/metros/radianos. Não transformar valores físicos arbitrários da FMU em códigos por cast implícito.

F-23 retém inputs no host, sem atuar em AO/DO/PWM por amostra inválida isolada. Dados de atuação só em STREAMING. No encerramento da execução, zerar saídas e cessar DATA; restart reinicializa a FMU e aplica outputs iniciais válidos. Zero físico significa DO baixo, DAC código zero nominal e PWM duty zero com nível inativo baixo. Tensão real e transitórios de reset exigem ensaio; níveis anteriores à execução do firmware não são garantidos pelo software.


## Representação e calibração

O mapa analógico usa volts; DAC converte para código de 8 bits e nunca confunde 0…255 com volts. ADC usa calibração correspondente à atenuação/resolução. PWM usa frequência e duty, com escala de mapeamento explícita; validar os limites configurados antes de Play.

## Dois núcleos e observabilidade da USB

Uso dos dois núcleos confirmado. Proposta: reservar afinidade da aquisição/atuação em um núcleo e comunicação/supervisão no outro; definir números de CPU e prioridades após inspecionar tarefas/IRQs do SDK escolhido. Memória e periféricos compartilhados ainda podem produzir interferência. Ensaiar carga máxima de comunicação e supervisão concorrentes com o caminho periódico.

A ponte CH340 é o dispositivo USB; ESP32 vê UART e não diretamente o read Linux. READ_ACK MID 03 confirma cumulativamente o último SEQ DAQC→host consumido pela aplicação. Não confundir esvaziamento de FIFO UART com leitura pela aplicação. RTS/CTS fica desabilitado e não substitui essa confirmação. A cadência do ACK e a tolerância de supervisão serão dimensionadas e medidas; não há CRC ou retransmissão de DATA.

## Baseline de firmware e UART

DEC-006 fixa ESP-IDF **v5.2.6** (`9ef24e3e2a2c96e720d83c574a3f8699177573da`), tag oficial da série 5.2 escolhida pelo usuário. O comando `idf.py --list-targets` deste SDK listou `esp32` no host em 09.09.2026. `micro_ros_setup`, `micro_ros_msgs` e Agent Humble foram compilados no host; o componente ESP-IDF Humble permanece em `4ddd8c26e721662319ed8af981cb7cdc9ae05382` até o build integrado. Não trocar para ramo rolling ou outra série ESP-IDF sem nova decisão. O MTU XRCE selecionado é 128 bytes; um quadro MID 04 completo de 133 bytes ocupa aproximadamente 8,75 ms em UART 8N1 a 152.000 bit/s. Esse é um limite aritmético do enlace, não uma medição de latência ou deadline.

A configuração do enlace é UART 8N1, sem RTS/CTS, configurável de 9.600 a 152.000 bit/s e aplicada simetricamente à ponte e ao ESP32. A baseline selecionada é 152.000 bit/s. Como 152.000 bit/s não é um valor `termios` POSIX convencional, o coordenador host usa `termios2` com `BOTHER`; a [evidência HOST](evidence/host-uart-152000-2026-09-09.md) confirma a taxa solicitada em pseudo-terminal. A compatibilidade CH340–ESP32, a taxa efetiva e o erro de taxa exigem ensaio em placa. A faixa não garante throughput: antes de STREAMING, validar o orçamento do frame e do passo. Para o quadro DATA máximo de 261 bytes, 152.000 bit/s representa aproximadamente 17,17 ms de transmissão serial 8N1; logo, uma execução de 100 Hz só é possível com payloads suficientemente menores e com orçamento completo medido.

## Opções configuráveis do perfil ESP32

Design autorizado pelo usuário; fontes descrevem ESP32 clássico e ESP-IDF 4.4.4 como referência técnica, sem escolher silenciosamente a versão final do firmware.

| ADC — opção | Limite/condição |
|---|---|
| Canal | Somente GPIO ADC exposto e disponível na tabela do perfil |
| Resolução em leitura individual | 9, 10, 11 ou 12 bits; ADC1 compartilha largura por unidade, não oferecer escolhas incompatíveis simultâneas |
| Atenuação por canal | 0; 2,5; 6; 11 dB |
| Faixa de referência publicada | Respectivamente 0,100–0,950; 0,100–1,250; 0,150–1,750; 0,150–2,450 V |
| Unidade e mapa | Volts; escala/offset para unidade da entrada FMU; limites configurados compatíveis com faixa caracterizada |
| Calibração | Identificar eFuse/curva usada; não apresentar estimativa como medição calibrada |

As faixas são referências de medição, não limites absolutos elétricos nem promessa de precisão perto de zero. Sem caracterização adicional, não anunciar ADC universal de 0–3,3 V. ADC2 depende dos conflitos de rádio/SDK. Taxa de aquisição depende do conjunto de canais e do orçamento medido, não da resolução escolhida. Fonte: [ADC Espressif](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/adc.html).

| PWM — opção | Limite/condição |
|---|---|
| Gerador do perfil | LEDC, GPIO de saída disponível; até 16 canais de hardware sujeitos a recursos livres |
| Frequência | Inteira em Hz, 1…40.000.000 como envelope de configuração, aceitando somente pares realizáveis pelo clock/divisor/resolução; 1 Hz não é mínimo universal do silício |
| Resolução | 1…20 bits no ESP32 clássico, limitada pela frequência e clock; mostrar opções compatíveis, não produto cartesiano irrestrito |
| Duty | Usuário informa 0…100%; codificação normalizada 0…1 e quantização compatível; extremos 0/100% exigem tratamento correto do driver sem overflow |
| Timer | Canais compartilhando timer compartilham frequência/resolução; rejeitar conflitos antes de Play |
| Exemplos de restrição | 5 kHz admite até 13 bits no exemplo oficial; 40 MHz só admite 1 bit e duty oscilante fixo de 50% |

Não configurar clocks/periféricos no ciclo periódico. GUI deve mostrar frequência efetiva quantizada e erro relativo antes de aplicar configuração, sem reduzir resolução silenciosamente. Fixar nível baixo no encerramento é estado de saída, não pedido de PWM de 0 Hz. Fonte: [LEDC Espressif](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/ledc.html). A faixa oferecida será intersectada com o driver/clock efetivamente selecionado e validada em bancada.
