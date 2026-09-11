# Evidência de bancada — firmware ESP32 e CONFIG pela CH340

Data: 11.09.2026. Ambiente: Ubuntu 22.04, ESP-IDF v5.2.6, placa detectada pelo driver CH341 como `/dev/ttyUSB0`, ESP32-D0WD-V3 revisão 3.1, dual-core de 240 MHz e flash de 2 MB. Escopo: gravação e protocolo CONFIG em estado seguro.

## Preparação executada

1. Compilar `firmware/daqc_esp32` para o alvo `esp32` com ESP-IDF v5.2.6.
2. Identificar que a configuração gerada inicialmente ainda mantinha console e logs de boot na UART0, compartilhada com o protocolo CH340.
3. Versionar em `sdkconfig.defaults` a seleção explícita de `esp32`, console desabilitado, nível de log da aplicação zero e log de bootloader zero; regenerar `sdkconfig` e recompilar.
4. Confirmar no `sdkconfig` regenerado `CONFIG_ESP_CONSOLE_NONE=y`, `CONFIG_LOG_DEFAULT_LEVEL_NONE=y` e `CONFIG_BOOTLOADER_LOG_LEVEL_NONE=y`.
5. Gravar bootloader, tabela de partições e a imagem final pela CH340 a 460.800 bit/s. O gravador confirmou o hash de cada região gravada.

## Resultado observado

A imagem final `microhil_daqc_esp32.bin` ocupou `0x3c2b0` bytes; a menor partição de aplicação é `0x100000`, com `0xc3d50` bytes livres. Após o reset feito pelo gravador, um cliente temporário abriu a UART em 8N1 a 152.000 bit/s e enviou somente os comandos CONFIG abaixo:

| Comando enviado | Resposta recebida | Resultado |
|---|---|---|
| `59 72 01 02` (ENABLE) | `59 72 01 02 02` | Estado efetivo ENABLE confirmado |
| `59 72 01 01` (DISABLE) | `59 72 01 01 01` | Estado efetivo DISABLE confirmado |

O ensaio não solicitou `STREAMING`, não publicou `DaqcSetup`, não configurou perfil ADC/PWM e não enviou DATA. A placa foi deixada em DISABLE.

## Verificação documental associada

`node test/run_spec_tests.js` passou em 29/29 testes. A chamada usual de `onp-spec verify` e `onp-spec audit --ci` não iniciou por incompatibilidade ESM/CommonJS da instalação local da skill no Node 18.20.8. Com `--experimental-default-type=module`, o motor iniciou, mas a auditoria recusou as provas antigas de `host-build-foundation` e `host-run-logging` como desatualizadas. Portanto, esta revisão não declara `onp-spec audit --ci` aprovado.

## Limites

O resultado demonstra detecção física, gravação verificada, UART a 152.000 bit/s, parser CONFIG e retorno de estado em uma placa. Não demonstra micro-ROS Agent, MID 04, tópicos `/daqc_*`, aplicação de perfil, ADC/DAC/GPIO/PWM, DATA, READ_ACK por 60 s, medição temporal, Raspberry Pi, FMU, HIL ou segurança elétrica.
