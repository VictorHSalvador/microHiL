# Evidência HOST — entradas virtuais operacionais

Data: 11.09.2026. Ambiente: Ubuntu 22.04, GCC 11.4.0, CMake, Qt 6.2.4 e FMILibrary 3.0.4 na revisão fixa configurada pelo projeto. Escopo: software HOST.

## Procedimento executado

O teste `execution_session_lifecycle` localizou as entradas da fixture, publicou o inteiro virtual `23`, rejeitou o valor fracionário `23,5` e, após aplicar o perfil físico, verificou que `u_real` não aceita fonte virtual por estar ligada a `GPIO32_AI`. A sessão publica a fonte virtual no `InputState` depois da preparação e antes da criação da thread, ou durante a execução sob mutex curto.

A GUI foi compilada e iniciada em modo `offscreen`; ela expõe somente entradas sem mapa DAQC como controles virtuais.

## Resultado observado

**47/47 testes CTest passaram.** A compilação GUI terminou e o processo permaneceu ativo durante o período offscreen. Os valores virtuais não são serializados em YAML.

## Limites

Não houve interação visual humana, execução de Play pela GUI, medição da contenção de mutex sob carga, DAQC, CH340, ESP32, Agent micro-ROS, ROS 2 em enlace, bancada ou HIL.
