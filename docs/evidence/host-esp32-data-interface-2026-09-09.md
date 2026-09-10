# Evidência HOST — telemetria genérica ESP32

Data: 09.09.2026  
Ambiente: Ubuntu 22.04.5 x86-64; ROS 2 Humble em `/opt/ros/humble`; `colcon`; build e instalação temporários em `/tmp/microhil-ros-build` e `/tmp/microhil-ros-install`.

## Procedimento executado

1. Adicionar `Esp32Data.msg` ao pacote `microhil_interfaces`.
2. Declarar campos físicos genéricos para ADC, DAC, entrada digital, saída digital e duty PWM dos GPIOs expostos, excluindo UART0 (GPIO1/3).
3. Compilar `microhil_interfaces` com `colcon build`.
4. Consultar a interface gerada com `ros2 interface show microhil_interfaces/msg/Esp32Data`.

## Resultado observado

O pacote compilou sem erro e a interface gerada contém os campos `gpioNN_adc_v`, `gpioNN_dac_v`, `gpioNN_di`, `gpioNN_do` e `gpioNN_pwm_duty` definidos no ICD. O conjunto possui 186 bytes de campos antes da serialização XRCE, portanto não cabe em um único payload de 128 bytes.

## Limites

O resultado não configura GPIOs, não escolhe modos simultâneos, não mede a serialização CDR, não exercita fragmentação, nem executa firmware, micro-ROS, Agent, CH340 ou ESP32. Valores associados a funções não habilitadas por um mapa de perfil não possuem semântica de medição.
