# ESP32 - Detalhes e limitações do controlador

| Campo | Valor |
|---|---|
| Componente | ESP32-D0WDQ5, revisão 3, confirmado por `esptool` |
| Documento de origem | `ESP32.PDF`, *ESP32 Series Datasheet*, versão 3.0, Espressif Systems, 2019 |
| Escopo | Chip ESP32; não descreve por completo um módulo de desenvolvimento ou uma placa DAQC específica |
| Aplicação no MICROHIL | Controlador da DAQC, responsável pela interface de I/O e pelas comunicações definidas para a plataforma |

## 0. Placa observada

### 0.1. Identificação visual e elétrica confirmada

A fotografia recebida mostra uma placa de desenvolvimento compatível com o formato comercial geralmente anunciado como **ESP32 DevKit V1 de 30 pinos**, com conector **USB-C**, botões `EN` e `BOOT` e módulo ESP32 blindado com antena PCB integrada.

Essa identificação se aplica ao **formato da placa**. O fabricante e a revisão da placa permanecem não identificados, mas a conexão USB e a consulta ao bootloader confirmaram o conversor USB-serial, o chip, sua revisão e a memória flash descritos abaixo.

Para o desenvolvimento inicial do MICROHIL, esta placa pode ser referenciada provisoriamente como:

> **DAQC-PROT-ESP32-DEVKIT-30P-USB-C - identificação da placa por formato e identificação do chip confirmada por `esptool`.**

| Propriedade verificada | Resultado |
|---|---|
| Conversor USB-serial | QinHeng CH340/CH341, USB `1a86:7523` |
| Porta Linux | `/dev/ttyUSB0` |
| Chip | ESP32-D0WDQ5 |
| Revisão | 3 |
| CPU | Dual-core, até 240 MHz |
| Rádio | Wi-Fi e Bluetooth |
| Cristal | 40 MHz |
| Calibração de referência ADC | VRef em eFuse disponível |
| Esquema de codificação da flash | None |
| Flash SPI detectada | 4 MB |
| MAC Wi-Fi/Bluetooth | `b4:bf:e9:11:85:78` |

O comando `chip_id` emite o aviso de que o ESP32 não possui um identificador numérico de chip separado; nesse modelo a ferramenta retorna o MAC como identificador único. Isso é esperado.


### 0.2. Terminais expostos pela placa fotografada

A placa possui dois headers com 15 terminais cada, totalizando **30 terminais físicos**. Eles não correspondem a 30 GPIOs: 25 terminais estão ligados a GPIOs do ESP32 e os outros cinco são `EN`, dois `GND`, `VIN` e `3V3`.

| Lado esquerdo, de cima para baixo | Lado direito, de cima para baixo |
|---|---|
| `EN` | `D23` / GPIO23 |
| `VP` / GPIO36 | `D22` / GPIO22 |
| `VN` / GPIO39 | `TX0` / GPIO1 |
| `D34` / GPIO34 | `RX0` / GPIO3 |
| `D35` / GPIO35 | `D21` / GPIO21 |
| `D32` / GPIO32 | `D19` / GPIO19 |
| `D33` / GPIO33 | `D18` / GPIO18 |
| `D25` / GPIO25 | `D5` / GPIO5 |
| `D26` / GPIO26 | `D17` / GPIO17 |
| `D27` / GPIO27 | `D16` / GPIO16 |
| `D14` / GPIO14 | `D4` / GPIO4 |
| `D12` / GPIO12 | `D2` / GPIO2 |
| `D13` / GPIO13 | `D15` / GPIO15 |
| `GND` | `GND` |
| `VIN` | `3V3` |

`VP` e `VN` correspondem aos GPIO36 e GPIO39. A nomenclatura `Dxx` serigrafada na placa corresponde ao número de GPIO, e não a uma abstração independente como ocorre em algumas placas Arduino.

| Grupo | Quantidade | Terminais |
|---|---:|---|
| GPIOs expostos | 25 | GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO12, GPIO13, GPIO14, GPIO15, GPIO16, GPIO17, GPIO18, GPIO19, GPIO21, GPIO22, GPIO23, GPIO25, GPIO26, GPIO27, GPIO32, GPIO33, GPIO34, GPIO35, GPIO36 e GPIO39 |
| Controle | 1 | `EN` |
| Alimentação | 2 | `VIN` e `3V3` |
| Terra | 2 | Dois terminais `GND` |
| Total físico | 30 | 15 terminais em cada lado da placa |

Entre os 25 GPIOs expostos, GPIO34, GPIO35, GPIO36 e GPIO39 são somente entrada. Assim, a placa expõe **21 GPIOs capazes de atuar como saída digital** e **4 GPIOs exclusivamente de entrada**.

### 0.3. Recursos de I/O expostos e limitações específicas da placa

| Função | Pinos expostos | Observação para o MICROHIL |
|---|---|---|
| GPIO digital com entrada e saída | GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO12, GPIO13, GPIO14, GPIO15, GPIO16, GPIO17, GPIO18, GPIO19, GPIO21, GPIO22, GPIO23, GPIO25, GPIO26, GPIO27, GPIO32 e GPIO33 | São 21 pinos. Alguns possuem conflitos de boot, serial ou periféricos, detalhados abaixo. |
| GPIO exclusivamente de entrada | GPIO34, GPIO35, GPIO36/VP e GPIO39/VN | São 4 pinos. Não possuem capacidade de saída. |
| Entradas analógicas ADC1 | GPIO36/VP, GPIO39/VN, GPIO34, GPIO35, GPIO32 e GPIO33 | Preferenciais para aquisição analógica. Entre estes canais, GPIO34, GPIO35, GPIO36 e GPIO39 são somente entrada. |
| Entradas analógicas ADC2 | GPIO2, GPIO4, GPIO12, GPIO13, GPIO14, GPIO15, GPIO25, GPIO26 e GPIO27 | São 9 canais expostos. GPIO2, GPIO12 e GPIO15 exigem atenção durante o boot. O ADC2 também pode ficar indisponível ou sofrer conflito enquanto o Wi-Fi estiver ativo; para aquisição simultânea ao Wi-Fi, priorizar ADC1 e validar no framework usado. |
| DAC interno | GPIO25 e GPIO26 | Dois canais de 8 bits, com as limitações descritas na seção 2.4. |
| I2C sugerido | GPIO21/SDA e GPIO22/SCL | São os pinos mais usuais neste formato de placa. O mapeamento permanece configurável por software. |
| SPI sugerido | GPIO18/SCK, GPIO19/MISO, GPIO23/MOSI e GPIO5/CS | Disponíveis na placa, mas GPIO5 é pino de *strapping*. Verificar o nível aplicado por periféricos durante reset. |
| UART0 / programação | GPIO1/TX0 e GPIO3/RX0 | Associados ao USB-serial da placa; reservar para programação, logs e depuração enquanto possível. |
| UART2 sugerida | GPIO16/RX e GPIO17/TX | Alternativa usual para periféricos seriais, sujeita à confirmação da variante do módulo. |
| Alimentação | `VIN`, `3V3` e `GND` | A foto não permite confirmar a faixa aceita em `VIN`, a corrente disponível no regulador ou se `VIN` está ligado ao USB. Não alimentar cargas por esses pinos sem esquema da placa. |

### 0.4. Reservas recomendadas para o protótipo

- Reservar GPIO1 e GPIO3 para programação, console e diagnóstico até a arquitetura estar estabilizada.
- Não usar GPIO2, GPIO5, GPIO12 ou GPIO15 para cargas ou entradas externas que possam alterar seu nível durante reset. Eles são os *strapping pins* expostos nos headers desta placa. O GPIO0 também participa do boot do ESP32, mas não está exposto nos headers fotografados; ele é acionado pelo botão `BOOT` da placa.
- Usar GPIO34, GPIO35, GPIO36 e GPIO39 exclusivamente como entradas. Esses pinos são candidatos naturais para entradas analógicas, desde que a faixa elétrica seja condicionada para o ADC.
- Tratar GPIO25 e GPIO26 como os únicos candidatos da placa para DAC interno.
- GPIO16 e GPIO17 estão expostos nesta placa e podem ser considerados para UART2. Ainda é necessário verificar se há alguma conexão adicional específica da placa antes de congelar o mapeamento.

## 1. Capacidades principais

O ESP32 é um SoC de 2,4 GHz que integra Wi-Fi 802.11 b/g/n e Bluetooth 4.2 BR/EDR e BLE. A família possui processador Xtensa LX6 de 32 bits com uma ou duas CPUs, conforme a variante. As variantes D0WDQ6 e D0WD podem operar até 240 MHz; D2WD e S0WD têm limite de 160 MHz.

| Recurso | Capacidade indicada no datasheet | Relevância para a DAQC |
|---|---|---|
| Processador | Xtensa LX6, 32 bits, um ou dois núcleos; até 600 MIPS na família | Permite separar aquisição, atuação e comunicação, desde que o escalonamento seja verificado na implementação. |
| Memória interna | 448 KB ROM, 520 KB SRAM e 16 KB SRAM RTC | A SRAM é limitada para buffers extensos, histórico de dados ou mensagens grandes. Planejar o uso de memória estática e dimensionar buffers. |
| GPIO | O chip suporta até 34 GPIOs programáveis; a placa expõe 25 | Dos 25 GPIOs expostos, 21 aceitam entrada e saída e 4 são somente entrada. |
| ADC | Dois ADCs SAR de 12 bits; a placa expõe 15 canais analógicos | Há 6 canais ADC1 e 9 canais ADC2 nos headers. O ADC não deve ser tratado como conversor de instrumentação de alta precisão. |
| DAC | Dois DACs internos de 8 bits: GPIO25/DAC_1 e GPIO26/DAC_2 | Possui saída analógica real, mas com resolução de somente 8 bits. Para saídas HiL que exijam maior resolução, precisão ou faixa padronizada, usar DAC externo e circuito de condicionamento. |
| PWM | Motor PWM e LED PWM; LED PWM com até 16 canais | Útil para sinais PWM, mas PWM não substitui uma saída analógica sem filtragem e validação da resposta. |
| UART | Três UARTs, até 5 Mbps; DMA e controle de fluxo | Pode atender depuração, comunicação serial e integração com transceptores RS-232/RS-485 externos. |
| I2C | Dois controladores, mestre ou escravo; até 5 MHz condicionado pelo pull-up | Útil para DAC, expansores de I/O e sensores; a frequência útil depende do barramento físico. |
| SPI | Quatro SPI na família | Adequado para ADC/DAC externo, memória e periféricos de maior taxa. |
| I2S | Dois controladores com DMA | Pode ser usado como interface de fluxo digital especializado; não deve ser assumido como requisito da DAQC sem necessidade concreta. |
| CAN | CAN 2.0 | Requer transceptor CAN externo para o barramento físico. |
| Ethernet | MAC Ethernet 10/100 com DMA e IEEE 1588 | Exige PHY externo e vários sinais; não equivale a uma porta Ethernet pronta na placa. |
| Watchdog | Dois grupos de temporizadores, com watchdog principal em cada grupo; watchdog RTC | Recurso importante para recuperação de falhas, desde que a política de reset e estado seguro seja especificada. |
| ULP/RTC | Coprocessador ULP e memória RTC em deep sleep | Recurso voltado a baixo consumo; não é central para o ciclo HiL, mas pode ser útil em modos de espera. |

## 2. Limitações críticas para o MICROHIL

### 2.1. O datasheet é do chip, não da placa

O formato da placa, os 30 terminais, o CH340 e o chip ESP32-D0WDQ5 foram identificados. O fabricante e a revisão elétrica da placa permanecem desconhecidos. Portanto, a fotografia e o datasheet do chip não confirmam:

- qual circuito de alimentação e proteção existe;
- o esquema completo entre o CH340, os sinais de auto-reset e a UART0;
- a faixa de tensão efetivamente aceita pelos conectores de I/O;
- a presença de DAC, ADC externo, transceptores ou condicionamento analógico.

O `TARGET.md` do projeto deve registrar o módulo/placa exato, sua revisão e o mapa definitivo de pinos antes da definição do perfil da DAQC.

### 2.2. Alimentação e níveis lógicos

- A faixa operacional indicada para as alimentações é de **2,3 V a 3,6 V**; o valor recomendado é **3,3 V**.
- Tensões acima de **3,6 V** nos domínios de alimentação excedem o máximo absoluto do chip.
- A fonte externa deve ser dimensionada para pelo menos **0,5 A**, conforme o datasheet. Wi-Fi e Bluetooth podem elevar significativamente os picos de corrente: transmissão Wi-Fi pode atingir valores típicos de 180 a 240 mA, a depender do modo.
- As entradas e saídas do chip não são I/O de 5 V. Sinais de 5 V, industriais ou sujeitos a ruído/transientes exigem adaptação de nível, proteção e condicionamento externos.
- A corrente de saída declarada é uma característica elétrica de domínio e condição de teste, não autorização para alimentar cargas. Relés, válvulas, motores, LEDs de potência e interfaces de campo devem usar drivers externos.

### 2.3. ADC interno

- A resolução nominal é de 12 bits, porém a precisão é limitada por não linearidade, variação entre chips, ruído e configuração de atenuação.
- O datasheet informa variação de até **±6%** entre chips antes de calibração.
- Após calibração com referência de eFuse, o erro total informado chega a **±60 mV** na faixa efetiva de 150 a 2450 mV, na maior atenuação.
- Acima de aproximadamente **2,45 V** na maior atenuação, a precisão pode piorar.
- Para melhorar o resultado, o próprio datasheet recomenda múltiplas amostras, filtragem/média e calibração.
- A entrada analógica não recebe diretamente sinais de processo como 0-5 V, 0-10 V, ±10 V ou 4-20 mA. Cada faixa exige circuito de condicionamento, proteção e calibração próprios.

### 2.4. DAC interno

- Existem somente dois canais, em **GPIO25** e **GPIO26**.
- A resolução é de **8 bits**, ou seja, 256 níveis. Em uma faixa ideal de 0 a 3,3 V, o menor incremento teórico seria aproximadamente 12,9 mV; a resolução efetiva dependerá da alimentação, carga, ruído e circuito externo.
- O DAC usa a alimentação como referência e não é descrito como saída de precisão. Para uma DAQC HiL, não presumir que atende requisitos de exatidão, repetibilidade, ruído ou faixa bipolar sem ensaio de bancada.
- Para 0-10 V, ±10 V, 4-20 mA, mais canais ou maior resolução, prever DAC externo e estágio analógico apropriado.

### 2.5. GPIOs, boot e pinos reservados

- Entre os pinos expostos, GPIO34, GPIO35, GPIO36 e GPIO39 são somente entrada. Não devem ser planejados como saídas.
- Entre os pinos expostos nos headers, GPIO2, GPIO5, GPIO12/MTDI e GPIO15/MTDO são *strapping pins*. O nível elétrico presente durante reset influencia boot, configuração de `VDD_SDIO` ou log de boot. Não conectar cargas ou circuitos externos que possam forçar níveis incompatíveis durante a partida.
- O GPIO0 é outro *strapping pin* do chip, mas não é disponibilizado nos headers desta placa; o botão `BOOT` atua sobre ele.
- GPIO1/U0TXD e GPIO3/U0RXD são associados à UART0, normalmente importante para gravação e depuração. Seu reaproveitamento deve ser avaliado contra o método de programação da placa.
- GPIO6 a GPIO11 são associados à flash e não estão expostos nos headers da placa fotografada. Eles não fazem parte do perfil de I/O desta DAQC.
- A função de vários pinos é multiplexada. A escolha de ADC, DAC, touch, JTAG, UART, SPI, SDIO, Ethernet ou PWM pode disputar o mesmo GPIO.

### 2.6. Comunicação e tempo real

- O ESP32 possui recursos suficientes para comunicação serial, SPI, I2C e CAN, mas a disponibilidade física depende do módulo e do mapeamento de pinos.
- UART é uma interface lógica; RS-232 e RS-485 exigem transceptores externos.
- CAN 2.0 também exige transceptor externo.
- A interface Ethernet é somente MAC; requer PHY externo e interface MII/RMII.
- Wi-Fi e Bluetooth compartilham os recursos de rádio e variam consumo e atividade de sistema. Eles não devem fazer parte do caminho crítico do ciclo HiL sem uma análise temporal específica.
- O uso de micro-ROS não elimina a necessidade de definir transporte, limites de mensagem, timeouts, comportamento em desconexão e prioridade das tarefas.

## 3. Implicações para o perfil DAQC do MICROHIL

O perfil de DAQC deve declarar, para cada versão de hardware:

1. Modelo exato do chip, módulo e placa, incluindo revisão.
2. Lista de GPIOs disponíveis, reservados e proibidos.
3. Canais de entrada e saída, tipo de sinal, conector, unidade, faixa, resolução, precisão esperada e circuito de condicionamento.
4. Uso de ADC/DAC internos e a necessidade de conversores externos.
5. Interfaces habilitadas: USB/serial, micro-ROS, SPI, I2C, CAN ou Ethernet.
6. Layout de mensagens e mapeamento determinístico de cada I/O.
7. Política de inicialização, reset, watchdog, timeout de comunicação e estado seguro das saídas.
8. Limites de taxa de amostragem, latência, jitter e tamanho de buffers, validados em bancada.

## 4. Decisões ainda necessárias

| Decisão | Motivo |
|---|---|
| Fabricante, módulo comercial e revisão da PCB | O chip, a flash, o CH340 e o pinout já foram identificados, mas falta o esquema elétrico específico da placa. |
| Número de canais analógicos e requisitos de precisão | Determina se os ADCs/DACs internos são suficientes ou se serão usados conversores externos. |
| Faixas de I/O externas | Define os circuitos de proteção, condicionamento e isolamento necessários. |
| Transporte micro-ROS e papel da USB | Define se a conexão ao host é USB nativa, USB-UART, serial, Ethernet ou outra interface. |
| Política de falha e estado seguro | Necessária para garantir a resposta das saídas em boot, reset, watchdog e perda de comunicação. |


