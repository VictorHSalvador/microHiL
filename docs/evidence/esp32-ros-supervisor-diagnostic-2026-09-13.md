# Diagnóstico da supervisora ROS na DAQC — 13.09.2026

## Fato e alteração

Na fonte da revisão 0.26.6 que não recebeu confirmação CONFIG após a gravação, `app_main` criava a tarefa UART e então chamava `DaqcRosStart` de forma síncrona. `sdkconfig.defaults` configura a pilha de `app_main` em 4096 bytes. A função de início micro-ROS configura o transporte, inicializa `rcl`/`rclc` e cria a tarefa ROS; portanto executá-la nessa pilha é uma hipótese plausível de falha ou reset antes de a UART permanecer disponível, mas não uma causa demonstrada.

A revisão 0.26.7 removeu essa chamada síncrona. Depois de criar a tarefa UART, `app_main` cria e verifica a supervisora `daqc_ros_supervisor` no núcleo 0, com pilha de 8192 bytes e prioridade 3. Essa tarefa é a única chamadora de `DaqcRosStart`, tenta uma vez por segundo quando o estado não é STREAMING e não altera framing, MIDs, estados ou o ICD. A falha de criação da supervisora ou da tarefa I/O agora encerra `app_main` explicitamente.

## Verificações executadas

As verificações HOST desta revisão são inspeção estática da ordem de criação e testes do parser/harness independente do hardware. Elas não executam FreeRTOS, micro-ROS nem a imagem ESP32.

* `python3 tests/test_esp32_xrce_diagnostic.py` — 6 testes aprovados; cobre framing do harness e verifica estaticamente que `app_main` não chama `DaqcRosStart`, enquanto a supervisora preserva a pilha de 8192 bytes e a guarda fora de STREAMING.
* Build HOST isolado em `build-host-xrce-0266` — 46 CTests aprovados e 1 loopback XRCE pulado por pré-condição de ambiente.
* Build ESP-IDF v5.2.6 — bloqueado neste ambiente: o cache `build-xrce-0266` referencia `/tmp/esp-idf-v5.2.6`, que não está presente. Nenhuma imagem desta revisão foi produzida.
* Ensaio CH340 — não executado nesta revisão.

## Ensaio físico obrigatório

Gravar a imagem construída desta revisão, aplicar o reset de aplicação do harness e enviar CONFIG DISABLE `59 72 01 01`. Aceitar somente a confirmação completa `59 72 01 01 01`. Repetir uma vez sem reset para separar a transição de boot do enlace estável. Não executar XRCE, Agent, STREAMING ou I/O antes dessa baseline.

Se CONFIG responder, o resultado apenas exclui esta hipótese no cenário ensaiado; não confirma XRCE nem timing. Se não responder, preservar a captura bruta e coletar reset/boot e estado das tarefas antes de atribuir a falha à inicialização ROS.
