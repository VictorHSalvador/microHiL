# EV-CONFIG-TIMING-REVIEW — Revisão 0.5

Data: 06.09.2026. Escopo: Markdown, consultas a fontes Espressif e verificação estrutural no host. Nenhuma implementação, FMU, serviço ou firmware executado; evidências anteriores preservadas.

## Decisões e alterações

Respostas 1…5 e confirmação complementar incorporadas: ausência separada de invalidade, última atualização no momento de inserção FMI, leitor USB independente; zero dos atuadores no fim/Stop/Error e restart pela FMU reinicializada; ADC/PWM configuráveis com opções no TARGET; grade temporal fixa, sem compensação nem omissão de etapas FMI. Exemplo confirmado: cálculo 0→12 ms para h=10 ms, próxima etapa em 20 ms.

Design distingue etapas atrasadas, liberações não utilizadas e pior atraso; relatório apenas após execução. O tempo simulado permanece sequencial e pode atrasar relativamente ao relógio real. A transmissão/ack de encerramento ainda depende do ICD e não pode ser tratada como atuação física comprovada.

## Fontes

[ADC ESP-IDF 4.4.4](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/adc.html) e [LEDC ESP-IDF 4.4.4](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/ledc.html) fundamentam opções no TARGET. Documentação não fixa SDK final nem comprova calibração/timing da placa. O envelope de frequência apresentado é escolha de interface, filtrada pelas combinações realizáveis no clock/driver; não é promessa de que toda combinação seja válida.

## Verificação e limites

Conferência executada por Python: 59 requisitos únicos, 59 entradas de matriz, critério V/design/tarefa/pendência em cada requisito; links locais resolvidos; referências recebidas iguais a Downloads. git diff --check sem erros e diff de src/include/CMakeLists contra a base auditada vazio. Isso verifica estrutura documental, não ensaios de produto. Gate de implementação ainda depende da consolidação dos detalhes remanescentes do ICD e dos incrementos.
