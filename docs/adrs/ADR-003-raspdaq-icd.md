# ADR-003 — Consolidação do ICD usando o RaspDAQ

Data: 07.09.2026. Revisão documental 0.7. Origem: instrução explícita para usar o projeto RaspDAQ e seguir ao máximo sua arquitetura, sem CRC e sem recuperar amostras perdidas. Status: decisão aprovada para o contrato de comunicação; implementação e validação física ainda não executadas.

## Autoridade e referência

A cadeia principal observada é raspdaq-ffs.service → run_raspdaq_ffs.sh → raspdaq_main.py → raspdaq_ffs.py e RaspDAQNode(SharedDaqState). O CLI 8raspTest-MID-SEQ_Ciclic-Print.py é ferramenta de Sandbox e não prevalece sobre o serviço. LinuxHost_node.py é nó ROS de loopback, não driver USB do host.

As decisões MICROHIL prevalecem sobre os valores da referência: SYNC 0x7259, MID DATA=02 nos dois sentidos, COMMANDs 01/02/03, STATUS com códigos COMMAND, até 256 bytes úteis, timeout por transferência de no máximo 5 ms, grade fixa sem compensação, FMU genérica, Qt/debug, proteção no host e zero físico no encerramento.

## Decisão

O MICROHIL reutilizará o padrão arquitetural do RaspDAQ: coordenador único do enlace, instância persistente de estado compartilhado, snapshots imutáveis ou substituídos atomicamente sob seção crítica curta, controle processado durante streaming, schema posicional fixo por execução e número de sequência por direção. No HOST, um worker serial único possui a TTY e alterna RX e TX com operações limitadas; ele é separado da thread de simulação e impede leitores ou escritores concorrentes no enlace.

DATA é tráfego de tempo real sem confirmação individual, retransmissão, fila crescente ou recuperação. Pacote ausente, perdido, repetido, antigo, incompleto ou fora do prazo é descartado; o receptor segue para o próximo. Um salto de SEQ contabiliza a lacuna e aceita o pacote novo. No host, a thread da simulação usa o último snapshot publicado antes de inserir os inputs na FMU e mantém o último valor válido por canal conforme F-23/F-24.

Não haverá CRC no protocolo MICROHIL. O parser valida SYNC, MID, tamanho determinado pelo schema, estado e sequência. Sem CRC, alteração de bits que continue sintaticamente válida pode não ser detectada; a validação de tipo, finitude e faixa por canal continua obrigatória, mas não equivale a integridade do quadro. Essa limitação é aceita em favor do fluxo de baixa latência e deve constar nos testes e evidências.

O identificador de sessão no fio proposto na revisão 0.6 foi removido. Cada entrada em STREAMING limpa mailboxes e fragmentos pendentes e reinicia as sequências de ambas as direções. CONFIG é idempotente e confirmado pelo estado efetivo. DATA recebido fora de STREAMING é descartado. A ordenação do enlace e a limpeza coordenada impedem reaproveitar dados de uma execução anterior; isso deve ser testado, não presumido.

A confirmação cumulativa de leitura exigida por F-27 permanece uma extensão mínima. Ela informa apenas o último SEQ de DATA DAQC→host efetivamente consumido pela aplicação host. Não confirma validade numérica, não confirma atuação e não solicita retransmissão. Para não reduzir o payload útil de DATA, o ICD reserva MID 03 para READ_ACK no sentido host→DAQC. READ_ACK usa o mesmo contador uint16 e é coalescido: confirmações intermediárias podem ser substituídas pela mais recente.

O micro-ROS no ESP32 permanece requisito. Como esses componentes não existem no RaspDAQ, o ICD reserva MID 04 para XRCE nos dois sentidos. Um único coordenador C possui `/dev/ttyUSB*`/UART e demultiplexa CONFIG, DATA, READ_ACK e XRCE. O payload XRCE chega ao Agent por ponte UDP em loopback, sem segundo leitor serial. O transporte XRCE usa entrega best effort no caminho periódico; nenhum callback micro-ROS acessa diretamente a FMU ou concorre como segundo escritor dos atuadores.

## Resposta a cada parte do ICD

| Parte | Encontrado na referência | Resolução MICROHIL |
|---|---|---|
| Pacotes | struct little-endian; CONFIG 4/5 bytes; STATUS só CONFIG; DATA com SEQ uint16 e payload fixo | Adotar ordem/tamanho, bytes SYNC 59 72 e DATA 5+N; preservar códigos MICROHIL |
| Mapa | FieldSpec/PayloadSchema ordenados, formatos e offsets implícitos | Descritor explícito por GPIO, função, tipo, unidade, offset e escala, congelado por direção |
| micro-ROS | rclpy no Linux e FunctionFS; nenhum cliente/agente XRCE | Reusar coordenador e snapshots; multiplexar XRCE por MID 04 sem atribuir isso à referência |
| Sequência | uint16 modular, repeated/old/gap | Reusar comparação; gap aceita o pacote novo e apenas contabiliza perda |
| Corrupção/framing | SYNC/MID/tamanho/SEQ, explicitamente sem CRC no CLI | Parser acumulador UART, sem CRC e sem promessa de detectar toda alteração de bits |
| Início/fim | RX/TX separados, controle antes de DATA e rechecagem de estado antes de TX | Reusar ordem sem lock durante I/O; limpar mailboxes e sequência nas fronteiras |
| Filas/prioridades | Estado/cache mais recente e locks; escrita potencialmente bloqueante | Mailbox de última amostra, controle reservado e prazos limitados |
| Log/configuração | Nenhum formato binário MICROHIL encontrado | IF-LOG define formato host derivado dos requisitos |

## Limites e consequências

A UART do ESP32 não observa o endpoint USB do CH340 como o Linux gadget observa FunctionFS. O READ_ACK prova consumo pela aplicação host; esvaziar FIFO UART ou receber heartbeat não prova leitura. Uma leitura completa avança o ACK mesmo se algum canal contiver NaN, pois validade numérica é tratada separadamente no host.

SEQ uint16 serve para ordenar o fluxo corrente, detectar gaps e relacionar READ_ACK dentro de uma janela limitada. Não é índice do passo FMI, relógio remoto nem fila de recuperação. Cada início de STREAMING redefine a referência; o receptor não compara sequências de execuções diferentes. O watchdog de 60 s usa relógio monotônico e falta de avanço do ACK, não distância modular acumulada.

CONFIG é controle de estado e permanece idempotente. O host pode reenviar um comando enquanto não recebe a confirmação dentro do prazo de controle, mas não retransmite DATA nem paralisa o núcleo. Número máximo de tentativas e prazo agregado serão fixados após medir a ponte; cada transferência continua limitada a 5 ms.

O protocolo sem CRC não oferece integridade ponta a ponta. Frames com SYNC, MID ou tamanho inválidos são descartados e o acumulador procura o próximo início possível. Um frame sintaticamente válido com bits alterados pode chegar à validação de canal. A evidência de bancada deve registrar essa limitação.

## Resultado

A base CONFIG/DATA, a política de perda, a sequência e o ownership estão fechados para implementação incremental. READ_ACK e XRCE são os únicos MIDs adicionais necessários neste desenho; seus layouts estão no ICD. Permanecem dependentes de medição o baud rate, capacidades de fila, cadência de ACK e custo do micro-ROS. Nada nesta decisão comprova desempenho, compatibilidade física ou deadlines.
