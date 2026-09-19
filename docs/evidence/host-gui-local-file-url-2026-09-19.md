# EV-GUI-URL-LOCAL-2026-09-19

Data de execução: 19.09.2026. Ambiente: Ubuntu 22.04, ROS 2 Humble, Qt 6 e build ROS da GUI em `.local/microhil-gui-ros`.

## Correção verificada

Os diálogos QML deixaram de remover manualmente o prefixo `file://`. Eles enviam a URL intacta ao `GuiController`, que usa `QUrl` para obter o caminho local antes de carregar FMU ou YAML, salvar YAML ou exportar CSV. Em uma falha de carregamento de perfil, a mensagem agora inclui o caminho avaliado.

O teste `gui_controller_local_file_urls` foi adicionado e executado com:

```bash
ctest --test-dir .local/microhil-gui-ros --output-on-failure -R gui_controller_local_file_urls
```

Ele passou ao carregar, pelo `GuiController`, as URLs locais da FMU `MicroHiL_LoopbackTest.fmu` e do perfil `microhil_loopback_profile.yaml`, e confirmou que o caminho do perfil armazenado é local e normalizado. A suíte documental `node test/run_spec_tests.js` também passou com 29/29 testes.

## Limites

O teste é HOST e não abre uma janela interativa, a CH340, o Agent ou a DAQC. A inspeção visual manual do diálogo e o Play HiL físico após a correção continuam pendentes.
