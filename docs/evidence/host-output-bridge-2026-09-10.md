# Evidência HOST — bridge de atuação para DATA

Data: 10.09.2026  
Ambiente: Ubuntu 22.04 HOST; CMake; GCC; FMILibrary 3.0.4 obtida no diretório de build temporário.

## Procedimento executado

Foi compilado o projeto com runner e testes habilitados em uma árvore isolada e executado o CTest completo. O teste `daq_output_bridge_streaming` inicializa schemas fixos, o estado de inputs, o coordenador e a bridge. Ele tenta publicar antes de STREAMING, confirma ENABLE e STREAMING de forma simulada, publica uma saída digital e publica o payload de zero.

## Resultado observado

- Build concluído.
- CTest: 44 de 44 testes aprovados.
- A bridge rejeitou publicação antes de STREAMING.
- Após a confirmação, o coordenador forneceu um frame DATA de 26 bytes: prefixo de 5 bytes e payload de atuação de 21 bytes.
- O segundo frame DATA teve sequência seguinte e payload de zero.

## Limites

Esta é uma verificação HOST do coordenador e do mailbox. A confirmação foi simulada; não houve TTY, CH340, ESP32, firmware, micro-ROS, runner conectado, GPIO, tempo real, bancada ou HIL. Não demonstra que a DAQC recebeu, aplicou ou zerou nenhuma saída física.
