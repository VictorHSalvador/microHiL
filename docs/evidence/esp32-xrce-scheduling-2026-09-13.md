# Correção de espera XRCE na DAQC — 13.09.2026

## Alteração e verificação local

A tarefa de comunicação passou a consultar a UART sem bloqueio e, quando não há bytes, arma um `esp_timer` one-shot de 1 ms. O callback somente notifica a tarefa de comunicação. A espera tem uma guarda de 10 ms para o caso de a notificação não chegar; ao atingir a guarda ou ao falhar o armamento, uma flag local de falha é retida e o caminho degradado cede um tick, sem executar busy loop. A guarda não é o prazo nominal de transferência.

O parser do firmware agora rejeita `LENGTH` XRCE acima de 128 antes de formar o quadro. O novo harness `tools/esp32_xrce_diagnostic.py` mantém DTR baixo quando RTS é liberado no reset de aplicação, transmite CONFIG DISABLE `59 72 01 01`, exige a confirmação `59 72 01 01 01` antes e depois do ensaio, preserva captura bruta e reconhece DATA com 33 bytes. Ele não importa `pyserial` ao carregar o parser testado.

Foram executados em 13.09.2026:

1. `python3 tests/test_esp32_xrce_diagnostic.py`: 5 testes aprovados para CONFIG, DATA de 33 bytes contendo SYNC, READ_ACK, fragmentação/limite XRCE e reset de aplicação.
2. Build HOST isolado com `MICROHIL_BUILD_RUNNER=OFF`, `MICROHIL_BUILD_GUI=OFF` e `MICROHIL_WITH_ROS2_CONTROL=OFF`: 45 CTests aprovados e 1 loopback XRCE pulado por pré-condição de ambiente. O teste C adicional compila o parser de firmware real e cobre DATA com SYNC interno e recuperação após XRCE de 129 bytes.
3. Build ESP-IDF v5.2.6 isolado em `firmware/daqc_esp32/build-xrce-0266`: concluído; imagem `microhil_daqc_esp32.bin` de 245.664 bytes.
4. A imagem foi gravada em uma ESP32-D0WD-V3 pela CH340 `/dev/ttyUSB0`. O esptool confirmou a escrita e o hash de bootloader, aplicação e tabela de partições e aplicou hard reset.

## Ensaio físico e limite

Depois da gravação, o harness abriu a CH340 no grupo `dialout`, aplicou reset de aplicação e transmitiu CONFIG DISABLE. O comando observado no lado host foi `59720101`, mas não chegou a confirmação obrigatória `5972010101` em 2 s. A repetição sem reset também não recebeu confirmação. Portanto o ensaio não atingiu a baseline CONFIG e não mede MID 04, Agent, tópicos, latência ou a correção de escalonamento.

O diagnóstico de 0.26.5 não pode mais sustentar que a ausência de MID 04 ocorreu antes do firmware: seu procedimento usava a combinação DTR/RTS que pode entrar no bootloader e tratava DATA como cinco bytes. A ausência atual de confirmação CONFIG após a gravação é um bloqueio de transporte/inicialização a isolar antes de inferir o estado do cliente micro-ROS. O Agent antes disponível em `/tmp/microhil-agent-ws` não estava presente neste ambiente, de forma que a etapa Agent ausente→presente não foi executada.
