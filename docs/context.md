# Contexto do MICROHIL

O MICROHIL é a etapa inicial de uma plataforma HiL de baixo custo que poderá evoluir para PIHIL. O desenvolvimento será retomado a partir de uma especificação verificável, preservando as partes úteis do protótipo em C.

## Produto pretendido

Primeiro host: máquina Linux Ubuntu 22.04 com ROS 2 Humble, conforme usuário. Alvo posterior: Raspberry Pi 4 de 2 GB. Produto genérico para FMUs FMI 2.0 Co-Simulation, sem planta fixa; núcleo C/FMILibrary, GUI Qt 6/C++ desacoplada e terminal debug. Bulk/libusb e micro-ROS na DAQC são escolhas confirmadas; sua arquitetura conjunta com CH340/protocolo próprio ainda precisa ser fechada. Meta: pelo menos 100 Hz para modelos compatíveis com o orçamento medido, sem garantia universal de tempo real.

O operador importa FMU, escolhe perfil DAQC ou execução sem hardware, configura I/O e execução, inicia/para, acompanha gráficos e inspeciona registros. O hardware troca sinais com o sistema sob teste. Registro e gráficos permanecem desacoplados da simulação. Configuração em binário; logging binário somente de saídas finais por passo; exportação CSV após término, interpretando tipos pela FMU. GUI de baixa prioridade inspirada no Linux Mint; no máximo 10 Hz de renderização. Perfil de DAQC inicial denominado ESP32, com ADC/DAC internos aceitos.

## Fronteiras

| Parte | Responsabilidade |
|---|---|
| Aplicação host | Configuração, orquestração, FMU, comunicação, interface e registros |
| FMU | Modelo e solver de co-simulação; seus tempos internos e dependências precisam ser medidos |
| DAQC | Aquisição/atuação e resposta local a falhas, segundo perfil a definir |
| Circuitos externos | Condicionamento, proteção, faixas, drivers e carga; não inferidos do firmware |
| Operador/equipe | Escolher a FMU de cada execução e seu mapa; definir limites e esclarecer políticas pendentes, sem fixar uma planta de produto |
| Ferramentas de verificação | Testes HOST, simulador, bancada e HIL com resultados distintos |

PIHIL, certificação, novos periféricos sem requisito e exemplos ESP8266/DS18B20 não fazem parte do incremento atual. O Demo é um exemplo do processo SDD, não um módulo do produto.

## Estado observado

O commit `c788a4283cf81f17e9a2956ae258487c7931590a` contém sete módulos C, sete headers, CMake, menu terminal, CSV e gnuplot. Não há ROS, firmware, perfil DAQC, GUI Qt ou testes de aceitação implementados. A [avaliação inicial](avaliacao-sdd-2026-09-06.md) detalha as divergências e permanece válida para esse código. Esta revisão incorpora as respostas DEC-001…012, sem corrigir os defeitos de código ali registrados. O usuário condicionou a implementação ao fechamento documental.

A [auditoria HOST](evidence/audit-2026-09-06/README.md) identificou ausência da dependência FMILibrary na configuração limpa, testou a fila sequencialmente e reproduziu falha de escrita escondida pelo logger. Nenhuma FMU ou hardware foi executado nessa auditoria. A FMU presente tem binário x86-64 e somente outputs, portanto não basta como fixture de entrada nem para demonstrar Raspberry Pi.

## Glossário

| Termo | Uso |
|---|---|
| SDD-processo | Specification-Driven Development; requisito → design → tarefa → código → teste → evidência |
| SDD-documento | Software Design Document; design registrado em arquitetura e contratos |
| HOST | Lógica e software executados no computador, sem comprovação elétrica da DAQC |
| Simulador | Componente ou ambiente controlado que representa I/O/tempo; não é bancada |
| Bancada | Placa real com circuito e instrumentos identificados |
| HIL | Hardware em malha com modelo e injeção de falhas |
| Perfil DAQC | Conjunto versionado de canais, recursos, unidades, limites e contrato |
| Informação fornecida | Relato/documento do usuário preservado com origem, sem alegar repetição do ensaio |
| Proposta | Design ou critério sugerido ainda não confirmado |
| Pendente | Informação/decisão necessária para o escopo indicado |
