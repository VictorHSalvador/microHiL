# ADR-003 — Consolidação do ICD usando o RaspDAQ

Data: 06.09.2026. Revisão documental 0.6. Origem: instrução explícita “Utilize o projeto RASPDAQ para responder todas essas perguntas”. Status: reuso/adaptações documentados; extensões sem equivalente na referência permanecem propostas técnicas identificadas. Não autoriza declarar implementação, compatibilidade física ou tempo real validados.

## Autoridade e referência

Versão principal observada: serviço raspdaq-ffs.service → run_raspdaq_ffs.sh → raspdaq_main.py → raspdaq_ffs.py e RaspDAQNode(SharedDaqState). O CLI 8raspTest-MID-SEQ_Ciclic-Print.py é ferramenta de Sandbox e não autoridade superior ao serviço; ele aceita MID STATUS=04, que não consta do codec do serviço. LinuxHost_node.py é nó ROS de loopback, não driver USB do host.

Instruções MICROHIL prevalecem sobre números/semânticas RaspDAQ: SYNC 0x7259, MID DATA=02 nos dois sentidos, três COMMANDs 01/02/03, STATUS com códigos COMMAND, até 256 bytes úteis, timeout por transferência ≤5 ms, grade fixa sem compensação, FMU genérica, Qt/debug, proteção no host e zero físico no fim.

## Resposta a cada parte do ICD

| Parte | Encontrado na referência | Resolução MICROHIL |
|---|---|---|
| Pacotes | struct little-endian; CONFIG 4/5 bytes; STATUS só CONFIG no serviço; DATA tem SEQ uint16 e payload fixo | Adotar ordem/tamanho de controle; bytes SYNC 59 72. DATA base 5+N com N≤256; preservar códigos MICROHIL |
| Mapa | FieldSpec e PayloadSchema ordenados, formatos f/d, offsets implícitos pela lista | Descritor explícito por GPIO/função/tipo/unidade/offset, congelado por direção; não copiar campos de navegação nem defaults zero |
| micro-ROS | Serviço usa rclpy no Linux e FunctionFS; nenhum cliente/agente XRCE nesses componentes | Reusar processo coordenador e snapshots; proposta E-XRCE com dono único e callbacks cliente/agente; não confundir bridge ROS com micro-ROS implementado |
| Sequência/ACK | uint16 modular, repeated/old/gap; resposta CONFIG não é confirmação cumulativa de DATA | Reusar comparação dentro de janela válida; E-SESSION/E-ACK para confirmação aprovada pelo usuário. A referência não define 60 s nem SID no fio |
| Corrupção/framing | SYNC/MID/len/seq; explicitamente sem CRC no CLI; read direto ao decoder | Acumulador necessário para UART; CRC/length/version como E-INTEGRITY. Não alegar detecção de todo dado corrompido com esquema original |
| Início/fim | RX/TX separados, lock ordena CONFIG ACK antes do primeiro DATA, rechecagem de estado antes de TX, limpeza na parada | Reusar ordem, mas sem lock sob I/O ilimitado. ENABLE durante streaming encerra com zero; DISABLE explícito encerra; não copiar quarto comando STOP_STREAM |
| Filas/prioridades | Estado/cache mais recente e locks; ausência de cotas explícitas e escritas potencialmente bloqueantes | Último DATA substituível, controle reservado prioritário, XRCE separado; limites e prazos dependem do quadro integrado/medição |
| Log/configuração | Não encontrado formato binário compatível nos componentes inspecionados | IF-LOG propõe cabeçalhos e serialização portável derivados dos requisitos, não atribuídos ao RaspDAQ |

## Limites que não podem ser omitidos

A UART do ESP32 não observa o endpoint USB do CH340 como o Linux gadget observa FunctionFS. O serviço e o CLI RaspDAQ não configuram essa ponte. Não importar VID/PID, chamadas de inicialização, endereço de endpoint ou baud rate como configuração da placa atual.

O contador uint16 tem ambiguidade após meia faixa; sem nova garantia de janela, sessão ou contador maior, não basta para 60 s em qualquer taxa. CRC não existe na base; não anunciar integridade ponta a ponta. Um read tardio local não revela sozinho a idade da aquisição remota. Formato de sessão/ACK/XRCE/CRC exige extensão real; nenhuma escolha de byte adicional foi encontrada magicamente no projeto de referência.

## Próximo incremento documental concreto

Revisar em conjunto o formato estendido (sessão, ACK, integridade e canal XRCE), em vez de adicionar campos incompatíveis separadamente. A base e os vetores já documentados orientam reaproveitamento; não liberar parser/firmware definitivo baseado só neles. Implementação de HOST continua aguardando consolidação por instrução vigente; testes de layout em Python nesta revisão são verificação de documento, não execução do runtime.
