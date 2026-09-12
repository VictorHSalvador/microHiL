# Diagnóstico físico XRCE — ausência de MID 04

Data: 12.09.2026. Ambiente: ESP32-D0WD-V3 com a imagem atual, CH340 em 152.000 bit/s, Agent micro-ROS Humble recompilado temporariamente e ouvindo UDP local na porta 8888.

## Procedimento e resultado

1. O Agent Humble e `micro_ros_msgs` foram recompilados a partir dos ramos Humble em diretório temporário; o Agent iniciou em `udp4 -p 8888 -v6`.
2. Uma ponte temporária foi dona exclusiva da CH340. Ela demultiplexou somente MID 04, encaminhando payloads XRCE para o Agent e encapsulando respostas do Agent de volta no MID 04.
3. A DAQC foi reiniciada com a ponte já aberta. Durante 15 s, a ponte não observou nenhum quadro MID 04 e o Agent não registrou uma sessão.

## Conclusão e limites

A falha ocorre antes da ponte UDP, da sessão no Agent e dos tópicos ROS: o cliente micro-ROS atual não emitiu tráfego XRCE observável nesse ensaio. O diagnóstico não altera ICD nem infere uma causa. Não houve CONFIG adicional, ENABLE, STREAMING, DATA, READ_ACK, I/O, mudança de firmware ou HIL; a placa permanece em DISABLE.
