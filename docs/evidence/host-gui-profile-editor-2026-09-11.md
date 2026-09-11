# Evidência HOST — editor de perfil físico

Data: 11.09.2026. Ambiente: Ubuntu 22.04, GCC 11.4.0, CMake, Qt 6.2.4 e FMILibrary 3.0.4 na revisão fixa configurada pelo projeto. Escopo: software HOST.

## Procedimento executado

Foi compilada a GUI Qt Quick/QML em árvore temporária. A API C passou a validar um candidato YAML contra a FMU sem aplicá-lo; a GUI usa essa operação sobre arquivo temporário antes de gravar o destino final. O editor recebe entradas e saídas tipadas, oferece somente os canais físicos compatíveis e exige ADC/PWM explícitos ao salvar.

O CTest completo foi executado com uma fixture adicional que tenta associar `Integer` a `GPIO32_AI`. O carregador YAML a rejeitou.

## Resultado observado

A GUI compilou e iniciou em modo `offscreen`. **47/47 testes CTest passaram.** O teste de perfil cobre tanto a configuração ESP32 válida como a rejeição da associação física de `Integer`.

## Limites

Não houve interação visual humana nem gravação de um arquivo a partir da GUI. Não foram executados controles virtuais, DAQC, CH340, ESP32, Agent micro-ROS, ROS 2 em enlace, bancada ou HIL. A compilação não demonstra que uma configuração ADC/PWM será aceita pela DAQC física.
