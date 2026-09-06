# EV-DIRECTION-REVIEW — Correção da direção dos sinais

Data: 06.09.2026. Revisão 0.4. Escopo: edição/revisão de Markdown no host; nenhum código, serviço, FMU ou firmware executado. Estado anterior: revisão 0.3 local, sobre HEAD documental 0fc7aaf52e4ff0634ff1363c6a0e0e311e881333. Evidências anteriores preservadas como histórico.

## Decisões incorporadas

- Aquisição: mundo real → AI/DI DAQC → USB → host → inputs FMU. Atuação: outputs FMU → USB → AO/DO/PWM DAQC → mundo real.
- F-23: último válido por canal adquirido retido no host. Sem histórico, inicial válido do input FMU, conforme resposta explícita; não fabricar zero.
- F-24: checkbox mantido; no 100º passo consecutivo inválido por canal, Error, identificação dos canais/input FMU e novo Play. Não contar pacotes como passos, não somar canais e não disparar por constantes válidas.
- F-27: STREAMING sem progresso de leitura durante 60 s → DISABLE, sem zeramento. Confirmação cumulativa pelo host aprovada. Layout/cadência, sessão e tolerância ainda não aprovados como bytes/valores oficiais.
- Uso dos dois núcleos ESP32 confirmado. Distribuição exata das tarefas ainda proposta; supervisão e confirmação não bloqueiam o ciclo FMU/aquisição. Efeito temporal exige medição.

## Verificação documental executada

Procedimento: inspeção dos trechos afetados, buscas de regras antigas, contagem/comparação de IDs e critérios por Python, resolução de links locais e git diff --check. Comparação byte a byte das referências contra Downloads e comparação src/include/CMakeLists contra a base de código auditada.

Resultado: 59 requisitos únicos e 59 entradas correspondentes na rastreabilidade; cada requisito com critério V, design, tarefa e pendência. Links locais válidos; referências recebidas idênticas; nenhuma mudança em código de produção. Uma substituição ampla de lista foi detectada durante edição e corrigida antes da conferência final; as demais decisões de ADR-002 foram preservadas. Nenhum teste de produto foi declarado executado.

## Fontes técnicas e limites

[ESP-IDF conexão serial](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/establish-serial-connection.html) e [UART ESP32](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/uart.html) fundamentam a distinção entre UART do MCU e USB da ponte. A conclusão de que TX UART não comprova leitura pela aplicação é inferência da topologia, não observação instrumental desta placa.

[FreeRTOS ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/system/freertos.html) documenta afinidade de tarefas. A distribuição de aquisição/atuação e comunicação/supervisão entre núcleos é proposta de design, sem SDK definitivo escolhido ou garantia temporal medida.

A evidência valida somente a revisão documental no escopo descrito. G-CONSOLIDACAO continua aberto por detalhes remanescentes; não foi reaberto o limite de 100 passos, o checkbox, a referência inicial ou o período de 60 s já confirmados.
