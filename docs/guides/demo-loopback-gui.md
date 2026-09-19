# Demonstração HiL pela GUI com três loopbacks no ESP32

Este procedimento executa a FMU de exemplo em tempo real pela GUI Qt, envia três comandos físicos à DAQC e devolve as leituras à FMU. Ele foi preparado para Ubuntu 22.04 com ROS 2 Humble, ESP32-D0WD-V3 pela CH340 e porta `/dev/ttyUSB0`.

## 1. Pré-condições

- ROS 2 Humble instalado em `/opt/ros/humble`.
- ESP32 com o firmware MICROHIL compatível já gravado. A demonstração física validada usou a imagem 0.26.7 descrita em [esp32-streaming-smoke-2026-09-13](../evidence/esp32-streaming-smoke-2026-09-13.md).
- Usuário com acesso à CH340. Confira com `ls -l /dev/ttyUSB0` e `groups`. Se ocorrer `Permission denied` e o usuário não estiver no grupo `dialout`, execute `sudo usermod -aG dialout "$USER"`, encerre a sessão gráfica e entre novamente.
- Qt 6, libyaml, ferramentas de compilação, capability e `colcon` instalados:

```bash
sudo apt update
sudo apt install build-essential cmake git libyaml-dev libcap2-bin python3-colcon-common-extensions \
  qt6-base-dev qt6-declarative-dev qml6-module-qtquick qml6-module-qtquick-controls \
  qml6-module-qtquick-layouts qml6-module-qtquick-dialogs qml6-module-qtqml-workerscript \
  qml6-module-qtquick-templates qml6-module-qtquick-window
```

Defina o caminho absoluto do seu clone uma vez em cada terminal. Substitua `/caminho/para/microHiL` pelo diretório real no seu computador:

```bash
export MICROHIL_DIR="/caminho/para/microHiL"
cd "$MICROHIL_DIR"
```

## 2. Ligação física

Desconecte o USB antes de instalar os jumpers. Na placa usada, o número `Dxx` da serigrafia corresponde ao GPIO de mesmo número.

| Saída DAQC | Entrada DAQC | Jumper na serigrafia | Finalidade |
|---|---|---|---|
| GPIO16 DO | GPIO4 DI | `D16` → `D4` | Retorno digital |
| GPIO25 AO/DAC | GPIO32 AI/ADC | `D25` → `D32` | Retorno analógico |
| GPIO18 PWM | GPIO33 AI/ADC | `D18` → `D33` | Observação do nível PWM |

Depois de conferir os jumpers, conecte somente o USB-C da placa ao computador e confirme:

```bash
ls -l /dev/ttyUSB0
```

O jumper GPIO18→GPIO33 não mede duty PWM com precisão. Sem filtro passa-baixa, o ADC coleta amostras da portadora; o gráfico serve para observar atividade e níveis, não para qualificar duty, frequência ou ripple.

## 3. Construção persistente do Micro-ROS Agent

Esta etapa é necessária apenas quando `.local/microhil-agent-ws/install/setup.bash` não existe. O diretório `.local` é ignorado pelo Git e sobrevive a reinicializações do computador.

```bash
mkdir -p .local/microhil-agent-ws/src
git clone --branch humble --single-branch https://github.com/micro-ROS/micro-ROS-Agent.git .local/microhil-agent-ws/src/micro_ros_agent
git clone --branch humble --single-branch https://github.com/micro-ROS/micro_ros_msgs.git .local/microhil-agent-ws/src/micro_ros_msgs
cd .local/microhil-agent-ws
source /opt/ros/humble/setup.bash
colcon build --packages-select micro_ros_msgs micro_ros_agent
cd "$MICROHIL_DIR"
```

O procedimento validado e os commits observados estão em [micro-ros-agent-persistent-2026-09-19](../evidence/micro-ros-agent-persistent-2026-09-19.md).

## 4. Construção das interfaces e da aplicação

Gere primeiro as mensagens ROS 2 do projeto:

```bash
source /opt/ros/humble/setup.bash
colcon build --base-paths ros2 --build-base build-ros2 --install-base install-ros2 --packages-select microhil_interfaces
```

Configure e compile a GUI com ROS 2, FMILibrary e testes:

```bash
source /opt/ros/humble/setup.bash
source "$MICROHIL_DIR/install-ros2/setup.bash"
cmake -S . -B .local/microhil-gui-ros \
  -DMICROHIL_BUILD_GUI=ON \
  -DMICROHIL_BUILD_RUNNER=ON \
  -DMICROHIL_FETCH_FMILIB=ON \
  -DMICROHIL_WITH_ROS2_CONTROL=ON \
  -DBUILD_TESTING=ON
cmake --build .local/microhil-gui-ros --parallel 2
```

Play HiL exige `SCHED_FIFO`. Reaplique a capability depois de cada recompilação, pois o novo executável perde a capability do arquivo anterior:

```bash
sudo setcap cap_sys_nice=ep "$MICROHIL_DIR/.local/microhil-gui-ros/microhil_gui"
getcap "$MICROHIL_DIR/.local/microhil-gui-ros/microhil_gui"
```

O último comando deve mostrar `cap_sys_nice=ep`.

## 5. Execução

Use dois terminais. No terminal 1, mantenha o Agent aberto:

```bash
export MICROHIL_DIR="/caminho/para/microHiL"
cd "$MICROHIL_DIR"
source /opt/ros/humble/setup.bash
source "$MICROHIL_DIR/.local/microhil-agent-ws/install/setup.bash"
ros2 run micro_ros_agent micro_ros_agent udp4 -p 8888
```

Se aparecer `bind error` com `errno: 98`, a porta 8888 já está ocupada. Confira `ss -lunp | grep ':8888'`; use o Agent existente ou encerre explicitamente o processo antigo antes de iniciar outro.

No terminal 2, abra a GUI como usuário normal:

```bash
export MICROHIL_DIR="/caminho/para/microHiL"
cd "$MICROHIL_DIR"
source /opt/ros/humble/setup.bash
source "$MICROHIL_DIR/install-ros2/setup.bash"
"$MICROHIL_DIR/.local/microhil-gui-ros/microhil_gui"
```

Não execute a GUI inteira com `sudo`.

## 6. Carregamento da demonstração

Na seção **Modelo e perfil**:

1. Clique em **Importar FMU** e, dentro do seu clone, abra `tests/fixtures/MicroHiL_LoopbackTest.fmu`.
2. Clique em **Carregar perfil YAML** e, dentro do mesmo clone, abra `tests/fixtures/MicroHiL_LoopbackTest_profile.yaml`.
3. Confirme que a interface mostra seis mapeamentos e preenche:
   - `ao_feedback` → `GPIO32_AI`;
   - `pwm_feedback` → `GPIO33_AI`;
   - `di_feedback` → `GPIO4_DI`;
   - `ao_command_v` → `GPIO25_AO`;
   - `pwm_command` → `GPIO18_PWM`;
   - `do_command` → `GPIO16_DO`.

Na seção **Execução**, confirme o passo `0.01 s` e a duração `60 s` carregados do YAML. Mantenha **Registrar saídas** e **Atualizar gráficos** marcados. Na seção **Controle**, mantenha `/dev/ttyUSB0` como porta DAQC.

## 7. Configuração dos gráficos

Em **Saídas selecionadas para log e atuação**, marque as seis saídas. As três saídas de comando são necessárias para a atuação física; as três saídas com sufixo `_graph` permitem verificar o retorno.

Defina **Janela temporal comum** como `60`. Para cada saída, clique primeiro em **Configurar gráfico** e depois em **Abrir gráfico**:

| Saída | Mínimo Y | Máximo Y | Resolução Y |
|---|---:|---:|---:|
| `ao_command_v` | 0 | 3.3 | 0.5 |
| `pwm_command` | 0 | 1 | 0.1 |
| `do_command` | 0 | 1 | 1 |
| `ao_feedback_graph` | 0 | 3.3 | 0.5 |
| `pwm_feedback_graph` | 0 | 3.3 | 0.5 |
| `do_feedback_graph` | 0 | 1 | 1 |

As três primeiras janelas mostram os sinais originais produzidos pela FMU e enviados à DAQC. As três janelas com sufixo `_graph` mostram os sinais adquiridos nos pinos de retorno. Abra as seis janelas antes de iniciar a simulação. Cada janela guarda amostras somente enquanto estiver aberta e a GUI atualiza os gráficos em até 10 Hz.

## 8. Execução e resultado esperado

Clique em **Play HiL**. O preflight configura a DAQC e aguarda o estabelecimento ROS/XRCE antes de iniciar o relógio da FMU, portanto a simulação pode demorar alguns segundos para começar.

Ao final de 60 s, o resultado nominal esperado é:

- estado `Finished — complete`;
- 6.000 passos concluídos;
- DAQC encerrada em DISABLE;
- log binário fechado;
- `do_feedback_graph` alternando entre 0 e 1;
- `ao_feedback_graph` acompanhando, com as limitações do ADC/DAC, o comando analógico;
- `pwm_feedback_graph` mostrando amostras do sinal ligado diretamente ao ADC, sem interpretação quantitativa de duty.

Os 60 s e 6.000 passos configuram esta repetição da demonstração; somente a tela de resultado da execução confirma quantos passos foram efetivamente concluídos.

Deadlines, timeouts de leitura e valores elétricos devem ser lidos como resultados da execução atual. Os números históricos do ensaio de referência não são critérios universais de aprovação.

## 9. Encerramento e diagnóstico curto

Feche a GUI somente depois de a execução terminar ou use **Stop**. Interrompa o Agent no terminal 1 com `Ctrl+C` depois que a GUI tiver colocado a DAQC em DISABLE.

- `SCHED_FIFO priority ... could not be enabled`: reaplique `setcap` ao executável recompilado.
- `the YAML profile is incompatible...`: confirme que está usando a FMU e o YAML desta demonstração; a baseline 0.29.7 também corrige leitura numérica sob `pt_BR.UTF-8`.
- `/dev/ttyUSB0` ausente: reconecte a placa e execute `ls -l /dev/ttyUSB*`.
- `bind error ... errno: 98`: já existe um Agent na porta UDP 8888.
- Play HiL não habilitado: carregue um perfil YAML válido e confirme que a FMU foi importada.

Este tutorial reproduz uma demonstração funcional. Ele não qualifica precisão elétrica, carga, duty PWM intermediário, jitter, hard real-time ou execução no Raspberry Pi.
