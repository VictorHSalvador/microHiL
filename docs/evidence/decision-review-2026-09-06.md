# Verificação documental das respostas DEC

Revisão avaliada: 0.2, 06.09.2026. Executor: Codex nesta sessão. Escopo: atualização documental e inspeção estática de XML; não implementação ou teste de runtime.

## Alterações verificadas

- Respostas DEC-001…012 classificadas como confirmadas ou com detalhamento pendente; nove grupos Q preservam ambiguidades sem promover propostas.
- 28 textos de requisitos existentes atualizados frente à revisão 0.1; IDs de origem mantidos.
- Cinco IDs derivados adicionados: REQ-F-23, REQ-F-24, REQ-F-25, REQ-F-26, REQ-NF-32.
- Total: 58 requisitos, 58 linhas de matriz e 58 critérios de aceitação propostos.
- Requisitos novos rastreiam design/contrato/tarefas existentes; nenhum caso de aceitação recebeu resultado de aprovação de produto.
- Referências Markdown recebidas continuam idênticas byte a byte aos arquivos em Downloads; código em src/include e CMake sem diff.

## Procedimento e resultados

Python extraiu cabeçalhos de spec e linhas da matriz, verificou unicidade/completude dos IDs, presença de critério, existência de tarefas e perguntas e links locais. Comparação de texto com snapshot anterior da sessão identificou 28 revisões e cinco adições. `git diff --name-only -- src include CMakeLists.txt` retornou vazio.

Resultado estrutural: **aprovado no escopo documental**. Conferência final antes de adicionar este registro encontrou 71 links locais válidos. A primeira passagem encontrou link antigo do índice para DOCX não mais presente em docs/; o índice foi corrigido para apontar à referência Markdown preservada, sem restaurar/mover arquivos do usuário. Um falso positivo do verificador confundia Q-00 com parte de MICROHIL-REQ-001-A; a expressão foi corrigida para identificar Q somente como token e a verificação foi repetida.

## Inspeção adicional da FMU

Procedimento: abrir ZIP de `fmus/example1/ClosedLoopHiL_WGS84.fmu`, analisar `modelDescription.xml` com ElementTree e contar variáveis de causality output sem atributo start em seu elemento de tipo. Sem carregar/executar o binário.

Observado: **14 outputs, 12 sem start explícito**: ax, ay, az, omegaz, propellerFeedback, rudderFeedback, thetaz, vx, vy, x, y, z. Isso sustenta a necessidade de Q-04; não comprova valores calculados nem comportamento inicial da FMU em runtime.

## Consultas técnicas

Fontes primárias consultadas para dúvidas específicas; URLs também estão no TARGET/arquitetura/ICD:

- [Espressif USB–UART](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/establish-serial-connection.html): topologia da ponte.
- [micro-ROS custom transports](https://github.com/micro-ROS/micro-ros.github.io/blob/master/_docs/tutorials/advanced/create_custom_transports/index.md): transporte customizado cliente/agente; não é validação do protocolo próprio.
- [FMI 2.0.4 ScalarVariable](https://raw.githubusercontent.com/modelica/fmi-standard/v2.0.4/schema/fmi2ScalarVariable.xsd): tipos/start e inicialização calculada.
- [ESP-IDF ADC 4.4.4](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/adc.html) e [DAC 4.4.4](https://docs.espressif.com/projects/esp-idf/en/v4.4.4/esp32/api-reference/peripherals/dac.html): capacidades e faixa dependente de configuração; consulta não fixa SDK do projeto.

Tentativa de leitura do PDF FMI 2.0.4 pela ferramenta web falhou no redirecionamento; foram consultados schema/headers oficiais, sem declarar leitura integral do PDF. A URL pública de tutorial micro-ROS retornou 404; o arquivo correspondente no repositório oficial foi usado.

## Limitações e continuidade

Busca por nomes RaspDAQ nas pastas Projects/Downloads não localizou código do projeto; sua arquitetura permanece relato do usuário. Nenhum dispositivo foi conectado, resetado ou programado. Não foram executados build, testes do runner, GUI, ROS ou medições de 100 Hz/5 ms. A etapa documental não corrige o logger nem aprova o protocolo. Desenvolvimento aguarda sanear as perguntas, conforme instrução do usuário.
