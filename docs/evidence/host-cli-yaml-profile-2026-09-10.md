# Evidência HOST — carregamento YAML no terminal

Data: 10.09.2026  
Ambiente: Ubuntu 22.04 HOST; runner compilado com FMILibrary 3.0.4; fixture `tests/fixtures/teste001.fmu`.

## Procedimento executado

No runner terminal, foram fornecidas sequencialmente as ações de carregar a FMU fixture, carregar `tests/fixtures/esp32_profile.yaml`, selecionar todos os outputs e exibir a configuração. O YAML contém `step_size_s: 0.01`, `stop_time_s: 5.0` e três mapeamentos do perfil ESP32.

## Resultado observado

- A FMU FMI 2.0 Co-Simulation foi carregada e listou três inputs e três outputs numéricos.
- O YAML foi aceito com `profile_id = 1` e três mapeamentos.
- A configuração passou a mostrar passo de 0,01 s, duração de 5 s, caminho do YAML, identificador do perfil e quantidade de mapeamentos.
- A seleção de outputs permaneceu explícita e pode ser verificada antes de Play.

## Limites

O procedimento não iniciou Play nem abriu a DAQC. Não houve comunicação CH340/ESP32, configuração ADC/PWM, bridge conectada ao runner, firmware, micro-ROS, GPIO, timing, bancada ou HIL. A interação automatizada por entrada padrão demonstra somente o fluxo de terminal da fixture.
