# Configuração ADC/PWM YAML no HOST — 10.09.2026

- Ambiente: Ubuntu 22.04; GCC 11.4; libyaml 0.2.2; FMILibrary 3.0.4 em diretório temporário de build.
- Escopo: formato confirmado para `adc` e `pwm` no perfil YAML ESP32, validação completa dos seis ADCs e dois PWMs, e retenção dos valores em `profile_daqc_configuration_t` no HOST.

## Procedimento executado

1. Carregar fixture YAML com resolução ADC, atenuação por GPIO e frequência/resolução de ambos os PWMs.
2. Verificar os valores materializados pelo carregador.
3. Tentar carregar fixture sem a seção obrigatória `adc` e verificar rejeição estrutural.
4. Compilar o runner e executar a suíte CTest no diretório temporário `/tmp/microhil-runner-check`.

## Resultado observado

A fixture válida foi aceita, os valores ADC/PWM esperados foram preservados e a fixture sem `adc` foi rejeitada. A suíte CTest concluiu **45/45** testes com sucesso.

## Limites

O ensaio não executou GUI, Agent micro-ROS, CH340, ESP32, publicação em `/daqc_setup`, aplicação de configuração na DAQC, caracterização elétrica, bancada, HIL ou medição temporal. A validação HOST não comprova que uma combinação PWM seja realizável no periférico; essa confirmação continua sendo responsabilidade da DAQC e do ensaio físico.
