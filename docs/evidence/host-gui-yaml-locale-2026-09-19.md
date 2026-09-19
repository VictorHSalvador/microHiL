# EV-GUI-YAML-LOCALE-2026-09-19

Data de execução: 19.09.2026. Ambiente: Ubuntu 22.04, ROS 2 Humble, Qt 6, `pt_BR.UTF-8` disponível e build ROS da GUI em `.local/microhil-gui-ros`.

## Falha reproduzida

O perfil `MicroHiL_LoopbackTest_profile.yaml` era aceito pelo runner e pelo teste Qt executados com localidade C, mas a GUI rejeitava o mesmo arquivo com `invalid YAML configuration schema`. A reprodução controlada do teste Qt com `LC_ALL=pt_BR.UTF-8` falhou antes da correção. O carregador usava `strtod`, que respeitava a localidade do processo e esperava vírgula decimal, enquanto o YAML persistia números com ponto, como `0.01`, `1.0` e `0.0`.

## Correção e verificação

O parser passa a converter escalares reais com uma localidade numérica C explícita e limitada à chamada, sem alterar a localidade global do processo. O gravador Qt configura `QTextStream` com `QLocale::c()`, garantindo ponto decimal no arquivo. O teste `gui_controller_local_file_urls` usa `QGuiApplication`, carrega a FMU e o perfil por URLs locais e é executado com `LC_ALL=pt_BR.UTF-8` e backend Qt offscreen.

Foram executados:

```bash
cmake --build .local/microhil-gui-ros --target test_gui_controller microhil_gui fmu_rt_runner --parallel 2
ctest --test-dir .local/microhil-gui-ros --output-on-failure -R gui_controller_local_file_urls
LC_ALL=pt_BR.UTF-8 QT_QPA_PLATFORM=offscreen .local/microhil-gui-ros/test_gui_controller /home/linuxvh/Projects/microHiL/tests/fixtures/MicroHiL_LoopbackTest.fmu /home/linuxvh/Projects/microHiL/tests/fixtures/MicroHiL_LoopbackTest_profile.yaml /home/linuxvh/Projects/microHiL/tests/fixtures/esp32_profile.yaml
```

Os dois testes terminaram com código zero. Antes da correção, o segundo comando direto terminava com `could not load YAML through a local file URL`.

## Limites

A verificação é HOST e offscreen. Ela não executa Agent, CH340, ESP32, I/O físico ou simulação HiL. O carregamento visual do perfil na janela da GUI após a correção permanece como confirmação manual.
