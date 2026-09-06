# ADR-002 Diretrizes de produto após respostas DEC-001 a DEC-012

Data: 06.09.2026. Status: decisões abaixo confirmadas; detalhes abertos em [decisions.md](../decisions.md). Revisão documental 0.2; sem implementação. Não declara o ICD completo.

## Decisões

- Manter Bulk/libusb e micro-ROS no ESP32 pela USB-C da placa atual. Avaliar reuso do padrão RaspDAQ de serviço/snapshot/mutex. Leitura Python é possibilidade, não substituição confirmada da API C.
- Qt 6/C++ desacoplado, núcleo operável via terminal debug, prioridade GUI inferior à simulação e aparência inspirada no Linux Mint. Prints próprios somente debug, fora do ciclo crítico.
- Perfil ESP32 com os recursos disponíveis do módulo; ADC/DAC internos aceitos e faixas nativas. Recursos multiplexados não são canais independentes simultâneos.
- Produto independente de planta, importação FMI 2.0 CS com nomes/tipos, passo e duração configuráveis. Meta pelo menos 100 Hz, com capacidade efetiva medida por FMU/alvo. Timeout USB máximo 5 ms; escopo da operação ainda aberto.
- Continuar após deadline perdido, reportar contagem e pior atraso. Usar start na inicialização pretendida e último valor válido em Stop/fim/erro/valores inválidos. Checkbox de proteção após mais de 100 inválidos sequenciais. Pausa automática versus encerramento ainda ambíguos.
- Primeiro host Linux Ubuntu 22.04 com ROS 2 Humble; futuro Raspberry Pi 4 de 2 GB, micro-ROS compatível.
- Log binário somente de saídas finais por passo; CSV apenas após encerramento. FMU fornece interpretação dos tipos. Metadados/ordem e valor bruto/aceito ainda pendentes.
- Gráfico por saída: limites e espaçamento Y; janela de tempo comum deslizante; máximo 10 Hz. Fechar destrói histórico; reabrir coleta dali em diante; abrir antes de Play permitido. Configurar e abrir usam controles distintos.
- Abrir FMU e abrir perfil binário são ações separadas. Validar perfil contra FMU/DAQ e informar divergência por campo.
- SYNC=0x7259 (2 bytes); MID CONFIG=0x01/DATA=0x02; COMMAND DISABLE=0x01/ENABLE=0x02/STREAMING=0x03. Até 256 bytes de payload; STATUS somente ESP32→host. CONFIG deve continuar sendo atendido em STREAMING e DISABLE deve interromper streaming, liberando o enlace.

## Consequências e limites

Revisar IDs existentes sem alterar as referências originais. Acrescentar IDs somente para comportamentos novos explícitos (retenção, proteção, debug, conversão e meta temporal). NF-22 troca BB77 por 0x7259; NF-25 limita payload, não frame inteiro; NF-09 passa de 10 Hz fixos para teto de 10 Hz; F-10/12 não exigem métricas no log por passo, mas estatísticas finais permanecem.

Uma classe de snapshots e mutex não comprova micro-ROS no dispositivo nem deadlines. Código RaspDAQ ainda não inspecionado. Não aplicar alocação por ciclo do exemplo diretamente à thread C crítica. Campos de protocolo, estado pré-modelo, tipo de pausa, precisão analógica e metadados de log ainda não aprovados implicitamente.
