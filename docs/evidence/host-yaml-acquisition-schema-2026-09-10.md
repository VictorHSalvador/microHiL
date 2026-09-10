# Evidência HOST — YAML e schema de aquisição

Data: 10.09.2026  
Ambiente: Ubuntu 22.04 HOST; CMake; GCC; FMILibrary 3.0.4 obtida no diretório de build temporário.

## Procedimento executado

Foi compilado o projeto com runner e testes habilitados, usando uma árvore de build isolada. Em seguida foi executado o CTest completo. O teste `fmu_model_inputs` carrega a fixture FMU FMI 2.0 Co-Simulation e a configuração YAML, resolve os nomes e tipos das variáveis para a FMU carregada e constrói o schema de aquisição ESP32.

## Resultado observado

- Build concluído.
- CTest: 42 de 42 testes aprovados.
- O schema de aquisição produzido para a fixture contém o canal `GPIO32_AI`, offset 4 e payload fixo de 28 bytes, conforme IF-MAP.
- O construtor ordena os campos pelo offset físico, portanto a ordem dos mapeamentos YAML não altera o layout DAQC→host.

## Limites

Esta é somente uma verificação HOST com fixture. Não houve comunicação com CH340, ESP32, firmware, Agent micro-ROS, GPIO, ADC, DAC, PWM, tempo real, bancada ou HIL. O schema host→DAQC de 21 bytes, as seções YAML completas de ADC/PWM e a integração do schema ao ciclo de simulação permanecem pendentes.
