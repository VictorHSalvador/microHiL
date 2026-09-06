# MICROHIL — Definição de Requisitos

| Campo | Valor |
|---|---|
| Referência | `MICROHIL-REQ-001-A` |
| Versão | 01 |
| Data | 24.08.2026 |
| Origem | Conversão de `MICROHIL-REQ-001-A.docx` |

## Registro de alterações

| Revisão | Data | Autor | Comentário |
|---|---|---|---|
| A | 24.08.2026 | EQUIPE MICROHIL | Primeira versão do documento de definição de requisitos do projeto MICROHIL. |

## 1. Requisitos

Os requisitos do projeto MICROHIL foram classificados como funcionais e não funcionais. Os requisitos funcionais descrevem as capacidades e comportamentos que a plataforma deve disponibilizar ao usuário e aos componentes integrados. Os requisitos não funcionais estabelecem restrições de arquitetura, desempenho, tecnologias, interfaces, concorrência, comunicação e organização do software.

## 1.1. Requisitos funcionais

### REQ-F-01 — Carregamento de FMU

A plataforma deve permitir o carregamento de modelos matemáticos de sistemas físicos em formato FMU 2.0, no modo Co-Simulation, para execução no ambiente MICROHIL.

### REQ-F-02 — Configuração de Entradas e Saídas

A plataforma deve permitir que o usuário associe variáveis de entrada e saída da FMU às entradas e saídas disponibilizadas pelo perfil de DAQC selecionado.

### REQ-F-03 — Seleção e Controle da DAQC

A plataforma deve permitir ao usuário selecionar um perfil de DAQC compatível e habilitar ou desabilitar sua utilização na simulação.

### REQ-F-04 — Persistência do Perfil de Configuração

A interface gráfica deve permitir salvar e carregar, em arquivo binário, o perfil de configuração da simulação, incluindo a configuração de I/Os, o mapeamento entre FMU e DAQC e os parâmetros de execução aplicáveis.

### REQ-F-05 — Configuração da Execução da Simulação

A janela de simulação deve permitir ao usuário definir o tempo total de simulação e o passo da simulação. A interface deve manter coerência entre os parâmetros temporalmente relacionados.

### REQ-F-06 — Início da Simulação

A plataforma deve iniciar a execução da simulação quando o usuário acionar o comando Play, desde que as configurações necessárias estejam válidas.

### REQ-F-07 — Parada da Simulação

A plataforma deve interromper a execução quando o usuário acionar o comando Stop e deve finalizar de forma consistente o armazenamento dos dados habilitados para registro.

### REQ-F-08 — Indicação do Estado da Simulação

A interface deve apresentar o estado corrente da simulação, contemplando, no mínimo, os estados Idle, Running, Error, Stopped e Finished.

### REQ-F-09 — Apresentação de Falhas

Quando ocorrer uma falha de execução ou comunicação detectável pela plataforma, a interface deve indicar o estado de erro e apresentar ao usuário uma descrição da falha registrada.

### REQ-F-10 — Habilitação do Logging

A janela de simulação deve permitir ao usuário habilitar ou desabilitar o registro das variáveis de saída da FMU e das métricas de desempenho em arquivo binário.

### REQ-F-11 — Registro Temporal das Saídas

Quando o logging estiver habilitado, a plataforma deve registrar as variáveis de saída selecionadas em cada instante de amostragem, juntamente com a referência temporal correspondente.

### REQ-F-12 — Registro das Métricas de Desempenho

Quando o logging estiver habilitado, a plataforma deve registrar as métricas temporais de execução e as métricas de falha previstas para análise posterior.

### REQ-F-13 — Apresentação dos Resultados de Desempenho

Ao término da simulação, ou quando ela for encerrada no estado Stopped, a interface deve apresentar, no mínimo: quantidade de deadlines perdidos; quantidade de timeouts da USB; tempo médio do ciclo de simulação; tempo médio de processamento da FMU; tempo médio de leitura USB; tempo médio de escrita USB; e o pior caso (worst-case) de cada métrica temporal.

### REQ-F-14 — Plotagem das Variáveis de Saída

A plataforma deve permitir ao usuário abrir gráficos das variáveis de saída durante a execução da simulação, utilizando uma janela individual para cada variável selecionada.

### REQ-F-15 — Configuração dos Gráficos

A interface deve permitir configurar os limites mínimo e máximo do eixo vertical e a resolução de visualização das janelas de plotagem.

### REQ-F-16 — Controle da Plotagem em Tempo Real

A janela de simulação deve disponibilizar um controle que permita habilitar ou desabilitar a atualização dos gráficos em tempo real sem interromper a execução da simulação.

### REQ-F-17 — Entradas Virtuais

A janela de simulação deve disponibilizar controles de entradas virtuais adequados ao tipo da variável, incluindo controles para valores reais, inteiros e booleanos, permitindo seu mapeamento às entradas da FMU.

### REQ-F-18 — Comunicação de Configuração com a DAQC

A plataforma deve suportar o modo de configuração do protocolo USB 2.0, utilizando frames no formato SYNC | MID | COMMAND no sentido MICROHIL→DAQC e SYNC | MID | COMMAND | STATUS no sentido DAQC→MICROHIL.

### REQ-F-19 — Comunicação de Streaming com a DAQC

A plataforma deve suportar o modo de streaming do protocolo USB 2.0 utilizando frames no formato SYNC | MID | PAYLOAD.

### REQ-F-20 — Controle de Estado da DAQC

A plataforma deve enviar comandos de controle à DAQC e interpretar o campo STATUS retornado para confirmar o estado efetivamente assumido pelo dispositivo.

### REQ-F-21 — Validação da Configuração antes da Execução

Antes de iniciar a simulação, a plataforma deve verificar se a FMU foi carregada, se os parâmetros mínimos de execução são válidos.

### REQ-F-22 — Finalização Consistente do Logging

Ao finalizar uma simulação por término normal, Stop ou erro recuperável, a plataforma deve fechar de forma consistente os arquivos de logging e preservar os dados registrados até o último ciclo concluído.

## 1.2. Requisitos não funcionais

### REQ-NF-01 — Plataforma de Execução

A aplicação principal do MICROHIL deve ser executável em Linux Ubuntu 22.04.

### REQ-NF-02 — ROS 2

A integração ROS 2 do projeto deve utilizar a distribuição ROS 2 Humble.

### REQ-NF-03 — Execução em Tempo Real da Simulação

O loop principal de simulação deve ser executado em thread dedicada, utilizando pthreads e política de escalonamento SCHED_FIFO com prioridade superior às threads não críticas.

### REQ-NF-04 — Thread de Leitura USB

A leitura USB deve ser executada em thread dedicada de alta prioridade, de forma contínua enquanto a DAQC estiver habilitada e houver variáveis da FMU mapeadas para entradas provenientes da DAQC.

### REQ-NF-05 — Transferência USB

A comunicação USB 2.0 entre a main board e a DAQC deve utilizar transferências do tipo Bulk.

### REQ-NF-06 — Biblioteca USB

A implementação da comunicação USB no código C da main board deve utilizar a biblioteca libusb.

### REQ-NF-07 — Escrita USB no Ciclo de Simulação

A escrita das saídas destinadas à DAQC deve ocorrer ao final do ciclo de simulação correspondente, preservando a ordem temporal entre aquisição de entradas, processamento da FMU e publicação das saídas.

### REQ-NF-08 — Proteção de Dados Compartilhados

O compartilhamento de dados entre as threads de leitura USB e de simulação deve utilizar double-buffering, com mutex restrito à troca/seleção do buffer e às regiões críticas necessárias.

### REQ-NF-09 — Atualização da Interface Gráfica

A atualização dos gráficos deve ocorrer em frequência fixa de 10 Hz, desacoplada da frequência do loop de simulação.

### REQ-NF-10 — Prioridade da Interface Gráfica

A interface gráfica deve ser executada em thread distinta das threads críticas de simulação e comunicação, utilizando prioridade não real-time ou inferior às threads críticas.

### REQ-NF-11 — Linguagem do Núcleo de Simulação

As funções responsáveis pelo núcleo de simulação, gerenciamento da FMU e execução da thread de simulação devem ser implementadas em linguagem C.

### REQ-NF-12 — Interface Gráfica

A interface gráfica deve ser implementada em C++ utilizando Qt 6.

### REQ-NF-13 — Biblioteca FMI

A manipulação de FMUs 2.0 deve utilizar a biblioteca FMILibrary (FMILib) ou camada de abstração construída sobre ela.

### REQ-NF-14 — Biblioteca de Abstração para FMU

O projeto deve possuir uma biblioteca complementar à FMILibrary para encapsular operações recorrentes, incluindo importação e destruição de FMUs, tratamento de logs/erros e identificação dos tipos das variáveis de entrada e saída.

### REQ-NF-15 — Arquitetura Modular

O software deve ser estruturado em módulos separados, no mínimo, para interface gráfica, núcleo de simulação/FMU e comunicações USB/ROS 2, reduzindo acoplamento entre subsistemas.

### REQ-NF-16 — Extensibilidade de Perfis de DAQC

A definição das características da DAQC deve ser modular e orientada a perfil, permitindo a inclusão de novos modelos de DAQC sem alteração extensiva do núcleo de simulação.

### REQ-NF-17 — Aplicação da DAQC em micro-ROS

A aplicação executada no ESP32 deve utilizar micro-ROS, com mensagens e tópicos compatíveis com a interface ROS 2 da main board.

### REQ-NF-18 — Organização dos Ambientes ROS

Os componentes ROS 2/micro-ROS da main board e da DAQC devem ser mantidos em diretórios próprios e isolados do núcleo principal da aplicação, quando aplicável.

### REQ-NF-19 — Compatibilidade das Mensagens de I/O

As mensagens de comunicação com a DAQC devem representar a quantidade e os tipos de I/O suportados pelo perfil do ESP32/DAQC, incluindo entradas e saídas digitais e analógicas.

### REQ-NF-20 — Codificação Numérica

Valores reais de 32 bits transmitidos pelo protocolo devem utilizar representação IEEE 754 float32.

### REQ-NF-21 — Estrutura do Protocolo por Perfil

A organização das variáveis no PAYLOAD USB deve ser definida pelo perfil da DAQC, de forma determinística e documentada, permitindo que main board e DAQC interpretem o mesmo layout.

### REQ-NF-22 — Campos de Sincronização do Protocolo

O protocolo deve utilizar um campo SYNC de dois bytes com valor hexadecimal BB77 e um campo MID de um byte. O MID deve distinguir, no mínimo, mensagens CONFIG e DATA, conforme a codificação definida para o projeto.

### REQ-NF-23 — Validação de Frames

O receptor deve validar os campos SYNC e MID antes de interpretar COMMAND, STATUS ou PAYLOAD.

### REQ-NF-24 — Comandos do Protocolo

O campo COMMAND deve possuir um byte e suportar os comandos definidos no projeto para Disable, Enable e Start Streaming.

### REQ-NF-25 — Tamanho Máximo do Frame de Dados

O frame de dados deve respeitar o limite máximo de 256 bytes definido para o protocolo do projeto.

### REQ-NF-26 — Convenções de Código C

No código C, as variáveis devem utilizar camelCase, as funções devem utilizar UPPER_CASE e os arquivos devem utilizar snake_case.

### REQ-NF-27 — Convenções de Código Python

Nos componentes implementados em Python, os nomes de funções e arquivos devem utilizar snake_case.

### REQ-NF-28 — Detecção de Deadline

O núcleo de simulação deve medir o tempo de cada ciclo e contabilizar como deadline perdido todo ciclo cujo tempo de execução ultrapasse o período configurado para a simulação.

### REQ-NF-29 — Detecção de Timeout USB

A camada de comunicação deve possuir critério temporal configurável para detectar e contabilizar timeouts de leitura ou escrita USB sem bloquear indefinidamente a thread de simulação.

### REQ-NF-30 — Isolamento de Falhas da Interface

Falhas ou atrasos na atualização da interface gráfica e na plotagem não devem bloquear nem alterar a cadência do loop principal de simulação.

### REQ-NF-31 — Consistência Temporal do Ciclo

Cada ciclo de simulação deve consumir um conjunto consistente de entradas, executar uma única etapa da FMU e publicar um conjunto correspondente de saídas antes do início do ciclo seguinte.

## 2. Pontos para revisão e decisões futuras

Esta seção consta no sumário do documento de origem, mas não contém itens preenchidos na versão convertida.
