# Evidência HOST — runner ROS 2 e interfaces locais

Data: 11.09.2026. Ambiente: Ubuntu 22.04, ROS 2 Humble, GCC 11.4.0, CMake, FMILibrary 3.0.4 na revisão fixa e interfaces `microhil_interfaces` geradas localmente em diretório temporário. Escopo: software HOST.

## Procedimento executado

1. Gerar o pacote `ros2/microhil_interfaces` por `colcon build` sob `/opt/ros/humble`.
2. Carregar o overlay temporário das interfaces.
3. Configurar o runner em diretório limpo com `MICROHIL_WITH_ROS2_CONTROL=ON`, `MICROHIL_BUILD_RUNNER=ON`, `MICROHIL_FETCH_FMILIB=ON` e testes habilitados.
4. Compilar e executar CTest.

## Resultado observado

A variante ROS compilou. **48/48 testes passaram**, incluindo `daqc_ros_control_state` e `daq_xrce_udp_bridge_loopback`. O resultado demonstra que o runner e as interfaces geradas são compatíveis no HOST para o caminho de controle compilado.

## Limites

Não havia dispositivo exposto como `/dev/ttyUSB*` ou `/dev/ttyACM*` neste ambiente. Não houve Agent remoto, sessão XRCE real, CH340, ESP32, CONFIG físico, FMU com DAQC, GPIO, Raspberry Pi, bancada, HIL ou medição de deadlines.
