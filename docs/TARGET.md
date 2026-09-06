# Alvo físico e ambiente

Status: perfil inicial denominado **ESP32**, com ADC/DAC internos e disponibilização dos I/Os disponíveis confirmados pelo usuário; mapa simultâneo/faixas de aquisição ainda em detalhamento. Fonte local: [ESP32-CONTROLADOR.md](references/ESP32-CONTROLADOR.md). As alegações de fotografia/esptool são informações recebidas; foto, comando completo, versão e saída bruta não foram inspecionados nesta sessão. Nenhuma nova consulta ao dispositivo foi executada.

## Inventário de origem

| Campo | Informação recebida | Limite/pendência |
|---|---|---|
| Placa | Formato DevKit V1, 30 pinos, USB-C | Fabricante, revisão e esquema desconhecidos |
| Identificador provisório | DAQC-PROT-ESP32-DEVKIT-30P-USB-C | Identifica protótipo, não perfil aprovado de produto |
| MCU | ESP32-D0WDQ5, revisão 3, informado por esptool | Preservar saída bruta antes de consolidar nomenclatura comercial/ECO |
| CPU/cristal | Dual-core, capacidade até 240 MHz; cristal informado 40 MHz | Clock configurado de firmware ainda não definido |
| USB–UART | CH340/CH341, VID:PID 1a86:7523 | Confirmar variante, driver, baud rate, reset e exclusividade do dispositivo |
| Porta | /dev/ttyUSB0 na identificação fornecida | Não é identificador persistente nem prova de presença atual |
| Flash | 4 MB informados | Partições, modo e frequência pendentes |
| ADC | Referência VRef em eFuse informada | Não comprova calibração de sistema ou exatidão da DAQC |
| Módulo | Blindagem/antena PCB no relato | Modelo comercial e conexões adicionais pendentes |
| Host produto | Raspberry Pi 4 com 2 GB, confirmado pelo usuário | Arquitetura do SO, kernel e recursos reservados pendentes; etapa inicial em máquina Linux Ubuntu 22.04/ROS 2 Humble |
| Host auditado | Ubuntu 22.04.5, x86-64, kernel 6.8.0-138-generic | Não representa o alvo Raspberry Pi |
| Software alvo | ROS 2 Humble, FMILibrary, Qt 6 conforme requisitos | Fixar versões/revisões, toolchain, ESP-IDF, micro-ROS e RTOS efetivos |

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

Para a placa descrita, o caminho a considerar é host USB → ponte CH340 → UART do ESP32. Não assumir USB CDC nativo de ESP32-S3. Bulk/libusb e micro-ROS estão mantidos por DEC-001; falta definir dono da ponte, agente XRCE e coexistência dos frames próprios. O uso de Python para leitura continua possibilidade, não troca aprovada do requisito C/libusb.

A Espressif documenta explicitamente a divisão USB–ponte–UART. O componente micro-ROS no ramo Humble documenta transporte UART customizado e agente serial, o que demonstra uma alternativa a avaliar, sem fixar versões ou compatibilidade desta placa. Fontes: [ESP-IDF conexão serial](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/establish-serial-connection.html), [componente micro-ROS Humble](https://github.com/micro-ROS/micro_ros_espidf_component/tree/humble). Consultadas em 06.09.2026; a página ESP-IDF stable então indicava v6.1, não escolhida como SDK do produto.

## Dados necessários para liberar o perfil

O nome do perfil é ESP32. Disponibilizar recursos do módulo não significa configurar todos ao mesmo tempo: GPIO25, por exemplo, não é um DAC e um canal digital independente simultaneamente. A proposta de perfil usa catálogo de capacidades e impede colisões de função no mesmo pino. Q-05 fecha reservas, atenuação/unidades e inclusão de PWM/periféricos além de DI/DO/AI/AO.

| Grupo de capacidade | Recurso recebido | Faixa/representação a documentar |
|---|---|---|
| Digital | 25 GPIOs expostos; 21 capazes de saída, 4 somente entrada | Booleano 0/1; domínio nominal 3,3 V, não tolerância a 5 V. Limiares elétricos seguem chip/circuito, não uma faixa analógica livre |
| ADC1 | GPIO32/33/34/35/36/39 | Conversor nominal 12 bits; atenuação e faixa calibrada precisam ser escolhidas |
| ADC2 | GPIO2/4/12/13/14/15/25/26/27 | Mesma decisão de faixa; conflitos de rádio/boot/função devem ser respeitados |
| DAC | GPIO25/26, interno 8 bits confirmado | Código 0…255 corresponde nominalmente a 0…VDD3P3_RTC; tensão útil sob carga precisa ser caracterizada |
| UART0 | GPIO1/3 | Recurso usado pela ponte CH340; não disponível como GPIO livre enquanto esse enlace estiver ativo |
| PWM/SPI/I2C e demais funções | Capacidades multiplexadas da placa | Inclusão no primeiro perfil não deduzida de “todos”; Q-05 |

Não existe uma faixa única “padrão 0…3,3 V” com a mesma precisão para todo ADC ESP32. Como referência técnica, a documentação ESP-IDF 4.4.4 recomenda 150…2450 mV para maior precisão com atenuação 11 dB; isso não fixa a atenuação deste firmware. O perfil deverá informar atenuação, calibração e unidade real transportada. Fontes primárias: [ADC](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/adc.html), [DAC](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/dac.html). Versão documental consultada, não SDK selecionado.

Para cada função habilitada: identidade, GPIO/conector, tipo/unidade, faixa e conversão, incompatibilidades, estado inicial e último válido, ciclo e procedimento de ensaio. Códigos crus ADC/DAC não equivalem automaticamente a valores da planta em volts/metros/radianos. Não transformar valores físicos arbitrários da FMU em códigos por cast implícito.

A retenção em Stop/fim/erro foi escolhida; condição antes de receber primeiro modelo e após reset continua Q-04. Histórico de pinout/foto/esptool permanece fonte recebida, não ensaio repetido. O documento está em consolidação e não autoriza implementação física nesta fase.

