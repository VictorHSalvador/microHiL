# Evidência HOST — YAML e schema de atuação

Data: 10.09.2026  
Ambiente: Ubuntu 22.04 HOST; CMake; GCC; FMILibrary 3.0.4 obtida no diretório de build temporário.

## Procedimento executado

Foi compilado o projeto com runner e testes habilitados em uma árvore isolada e executado o CTest completo. O teste `fmu_model_inputs` carrega uma FMU FMI 2.0 Co-Simulation e o YAML de perfil, resolve saídas por nome/tipo e constrói o schema host→DAQC. O teste `daq_actuation_payload` verifica a codificação de digital e analógico, a transformação inversa de escala/offset, a retenção e o payload de zero.

## Resultado observado

- Build concluído.
- CTest: 43 de 43 testes aprovados.
- O schema de atuação da fixture contém `GPIO16_DO` e `GPIO25_AO`, com payload fixo de 21 bytes e offsets 0 e 13.
- A primeira saída inválida sem histórico é recusada. Após uma saída válida, um valor inválido retém o último valor codificado.
- O payload de encerramento contém zero em todos os 21 bytes.

## Limites

Esta é somente uma verificação HOST com fixture. Não houve transmissão para CH340/ESP32, execução do firmware, Agent micro-ROS, escrita de GPIO, conversão DAC/PWM física, confirmação de estado, tempo real, bancada ou HIL. O ciclo de simulação ainda não publica esse payload no mailbox DATA.
