# Evidência HOST — mailbox XRCE do coordenador

Data: 10.09.2026  
Ambiente: Ubuntu 22.04.5 x86-64; GCC 11.4.0; build isolado em `/tmp/microhil-xrce-mailbox`.

## Procedimento executado

1. Configurar e compilar os componentes HOST com `MICROHIL_BUILD_RUNNER=OFF` e testes habilitados.
2. Executar o CTest completo.
3. No teste do coordenador, receber um frame XRCE MID 04, enfileirar um payload XRCE no sentido host→DAQC, publicar DATA e enfileirar DISABLE.
4. Verificar a ordem de transmissão CONFIG, DATA e XRCE, além do MID, tamanho e primeiro byte do payload XRCE de saída.

## Resultado observado

O CTest concluiu 40 testes com sucesso. O coordenador aceitou XRCE recebido por callback, manteve somente um XRCE pendente de até 128 bytes e o transmitiu depois de CONFIG e DATA. O teste verificou que XRCE não ultrapassou tráfego crítico já pendente.

## Limites

O teste não instancia Micro-ROS Agent, serialização DDS/XRCE real, ROS 2, CH340, ESP32, firmware, FMU ou I/O físico. Não mede latência, fragmentação XRCE de biblioteca, throughput da UART ou comportamento temporal do produto.
