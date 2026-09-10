# Dependências Qt Quick/QML — 10.09.2026

- Ambiente: Ubuntu 22.04; Qt 6.2.4; CMake 3.22; GUI `microhil_gui`.
- Escopo: compilação e carregamento de recursos QML em execução sem tela.

## Dependências observadas

`qt6-base-dev` e `qt6-declarative-dev` permitem configurar e compilar o alvo C++/QML. A execução requer os módulos QML que correspondem aos imports da interface: `qml6-module-qtquick`, `qml6-module-qtquick-controls`, `qml6-module-qtquick-layouts`, `qml6-module-qtquick-dialogs` e `qml6-module-qtqml-workerscript`.

O estilo Fusion selecionado por Qt Quick Controls 2 também requer `qml6-module-qtquick-templates`. Sem ele, `ApplicationWindow` não carrega o plugin `qtquicktemplates2plugin`.

## Limites

O build do executável não prova que todos os módulos QML de runtime estejam instalados. A execução sem tela apenas carrega a interface; ela não valida aparência, interação humana, FMU, DAQC, ROS, timing ou HIL.
