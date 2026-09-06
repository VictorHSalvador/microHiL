# ADR-002 Diretrizes de produto após respostas DEC-001 a DEC-012

Data: 06.09.2026. Status: decisões abaixo confirmadas; detalhes abertos em [decisions.md](../decisions.md). Revisão documental 0.5; sem implementação. Não declara o ICD completo.

## Decisões

- Manter Bulk/libusb e micro-ROS no ESP32 pela USB-C da placa atual. Avaliar reuso do padrão RaspDAQ de serviço/snapshot/mutex. Leitura Python é possibilidade, não substituição confirmada da API C.
- Qt 6/C++ desacoplado, núcleo operável via terminal debug, prioridade GUI inferior à simulação e aparência inspirada no Linux Mint. Prints próprios somente debug, fora do ciclo crítico.
- Perfil ESP32 com os recursos disponíveis do módulo; ADC/DAC internos aceitos, PWM incluído e mapa analógico em volts. Recursos multiplexados não são canais independentes simultâneos.
- Produto independente de planta, importação FMI 2.0 CS com nomes/tipos, passo e duração configuráveis. Meta pelo menos 100 Hz, com capacidade efetiva medida por FMU/alvo. Timeout USB máximo 5 ms por transferência; descartar atrasado e usar último válido no passo até fim. Valor constante não é falha.
- Continuar após deadline perdido, reportar contagem e pior atraso. Correção 0.4: aquisição DAQC inválida é filtrada no host antes do input FMU, retendo último válido; sem histórico, usar inicial válido desse input. Com checkbox, 100 passos consecutivos inválidos por canal terminam em Error, identificando canais/inputs afetados e permitindo novo Play. Não exigir zeramento físico. Após 60 s de STREAMING sem progresso da confirmação cumulativa de leitura host, DAQC entra em DISABLE.
- Primeiro host Linux Ubuntu 22.04 com ROS 2 Humble; futuro Raspberry Pi 4 de 2 GB, micro-ROS compatível.
- Log binário somente de saídas finais por passo; CSV apenas após encerramento. FMU fornece interpretação dos tipos. Registrar outputs brutos da FMU; falha de log mantém run com aviso. Representação tipada/ordem/layout ainda pendentes.
- Gráfico por saída: limites e espaçamento Y; janela de tempo comum deslizante; máximo 10 Hz. Fechar destrói histórico; reabrir coleta dali em diante; abrir antes de Play permitido. Configurar e abrir usam controles distintos.
- Abrir FMU e abrir perfil binário são ações separadas. Validar perfil contra FMU/DAQ e informar divergência por campo.
- SYNC=0x7259 (2 bytes); MID CONFIG=0x01/DATA=0x02; COMMAND DISABLE=0x01/ENABLE=0x02/STREAMING=0x03. Até 256 bytes de payload; STATUS somente ESP32→host, usando códigos COMMAND. CONFIG deve continuar sendo atendido em STREAMING e DISABLE deve interromper streaming, liberando o enlace.

## Consequências e limites

Revisar IDs existentes sem alterar as referências originais. Acrescentar IDs somente para comportamentos novos explícitos (retenção, proteção, debug, conversão e meta temporal). NF-22 troca BB77 por 0x7259; NF-25 limita payload, não frame inteiro; NF-09 passa de 10 Hz fixos para teto de 10 Hz; F-10/12 não exigem métricas no log por passo, mas estatísticas finais permanecem.

Uma classe de snapshots e mutex não comprova micro-ROS no dispositivo nem deadlines. Código RaspDAQ inspecionado estaticamente: um SharedDaqState duradouro e snapshots substituídos sob RLock; runtime rclpy/FunctionFS não demonstra micro-ROS no ESP32. Não aplicar alocação por ciclo do exemplo diretamente à thread C crítica. Q-03/Q-04/Q-08 resolvidas. Permanecem detalhes de framing/XRCE, validação de recursos ADC/PWM e correlação de pacotes, representação do log e layout/cadência/tolerância da supervisão; propostas não são decisões aprovadas.

Q-08 autoriza identificar e informar capacidades não suportadas no primeiro incremento. Q-05 confirma recursos selecionáveis com exclusões por GPIO/reserva UART. Rastrear REQ-F-27 para streaming sem progresso de leitura por 60 s, sem reusar timeouts didáticos.

Confirmações complementares: manter checkbox; sem histórico, usar valor inicial válido do input FMU; usar confirmação cumulativa de leitura emitida fora do núcleo; utilizar os dois núcleos ESP32. A distribuição exata das tarefas é proposta e deve ser medida. Revisão 0.3 permanece histórica, com direção física/zeramento e limite 101 substituídos.

Revisão 0.5: leitor USB em thread independente; consumir última atualização antes da inserção FMI; ausência conta timeout separado. Encerrar simulação zera AO/DO/PWM e encerra DATA, enquanto novo Play reaplica outputs iniciais da FMU reinicializada. Aguardar próximo instante da grade fixa após overrun, sem compensar nem alterar passos do modelo; relatório temporal somente no fim. ADC/PWM configuráveis dentro das opções explícitas do TARGET.
