# Plano de verificação

Revisão 0.2: procedimentos propostos para 58 requisitos; sem ensaio novo de produto nesta revisão. Implementação e execução desses testes aguardam fechamento documental solicitado. Cada `V-F-*`/`V-NF-*` em [spec.md](../spec.md) é um caso planejado associado ao mesmo número de requisito; [traceability.md](../traceability.md) identifica design, tarefa e evidência real disponível.

## Ambientes e limites

| Ambiente | Pode demonstrar | Não demonstra sozinho |
|---|---|---|
| HOST | Validação, lifecycle com doubles/fixtures, fila, erro de sink, parser, fórmulas, estados e serialização | Resposta elétrica e deadlines da DAQC |
| Simulador | Sequências, idade, relógio e falhas controladas | Jitter físico ou tolerância analógica |
| Bancada | Canais reais, reset/boot, timing elétrico e recursos no alvo | Toda a malha integrada sob condições ainda não exercitadas |
| HIL | Interação modelo–host–DAQC–sistema sob teste e falhas | Garantia universal além do perfil/carga ensaiados |

## Procedimentos por família

- **FMU/configuração:** fixture mínima com comportamento analítico conhecido e tipos reais/inteiros/booleanos; arquivos inválidos, FMI/kind incompatível, binário de arquitetura errada, ausência de variável, mapping conflitante, NaN/Inf/overflow e inicialização falha. Rodar repetidas execuções e conferir liberações. A FMU atual cobre somente outputs e não tem resultado matemático de referência estabelecido nesta revisão.
- **Fila/logger:** fila vazia/cheia, wrap, ordenação, produtor/consumidor concorrentes, finalização controlada entre leitura vazia e fim de produção, falha de abertura/escrita/flush/close e Stop/erro da simulação. Injetar erro no sink; não depender somente de disco realmente cheio. Confrontar sequência produzida, aceita, persistida e descartada.
- **Tempo:** clock/sleep injetáveis para verificar fórmulas, atraso antes/depois do sleep, continuidade após overrun, pior perda, último passo e Stop. Ensaiar referência 100 Hz/10 ms e timeout máximo 5 ms no escopo que Q-06 definir; separar configuração de tempo observado. Em alvo, medir sob carga definida de CPU, comunicação, disco e GUI; registrar scheduler/afinidade efetivos, duração e configuração. Medidas lógicas e elétricas devem ter referências explicitadas; médias e máximos não substituem limites de aceitação pendentes.
- **Interface:** vetores de referência de tipos/byte order/layout após DEC-012; fragmentação/agregação, ruído, tamanho inválido, frame parcial, timeout, sequência repetida/antiga/wrap, sessão e versão incompatível. Bulk/libusb e micro-ROS estão mantidos: testar ambos e a arquitetura XRCE definida em Q-01. Vetores do protocolo usam SYNC 0x7259; não usar BB77 como valor vigente. Testar payload 256 em frame maior, STATUS após esclarecimento, payload contendo SYNC e DISABLE sob streaming.
- **GUI/persistência:** interação durante execução, janelas por variável, limites Y, atualização independente de no máximo 10 Hz, desligar atualização sem parar o núcleo, salvar/carregar perfil, versão/corrupção e integridade de log. Conferir ticks −2…2 de 1 em 1, janela comum deslizante, abertura antes de Play e histórico destruído ao fechar/reabrir. Formato binário e CSV posterior confirmados; Q-07 fecha interpretação/metadados e reação à falha.
- **Firmware/bancada:** identificar circuito/instrumento e estimular canais dentro do perfil; medir calibração, atuação, boot/reset, watchdog, falta de dados, link e recuperação. Não presumir saída 0 V como estado seguro. Taxas, limites de erro, duração/repetições precisam ser aprovados antes do ensaio de aceitação.
- **Inicialização/proteção:** outputs com/sem start, primeira amostra inválida, retenção após NaN/Inf, limite 100/101, checkbox ligado/desligado, contagem/retomada conforme Q-03; stop/fim/erro, link perdido e reset separados. Capturar prints próprios: ausentes fora de debug; mensagens de FMU/código na GUI fora da thread crítica.
- **Conversão:** ler binário tipado com FMU correta e incorreta, seleção/ordem diferente e truncamento; converter apenas após encerramento e comparar amostras finais por passo. Não exigir métricas por passo no arquivo.
- **Estilo:** inspeção de símbolos próprios C/Python, comentários e quebras de linha, com exceções de ABI/terceiros. Revisão de comentário exige leitura humana/técnica; regex não determina se a frase é útil ou “comentário de IA”.

## Evidência mínima

ID de teste e requisito, commit e alterações locais, hash de FMU/firmware/fixture, versões, SO/arquitetura, perfil de placa, configuração, instrumentos/calibração aplicável, comando/procedimento, dados brutos, cálculo, esperado, observado, resultado, limitações e responsável pela execução. Aprovação de produto é distinta de execução do teste.

Reutilizar a pasta `docs/evidence/` por execução. Nenhum resultado planejado recebe arquivo EV com números inventados. A [auditoria inicial](../evidence/audit-2026-09-06/README.md) contém fila sequencial aprovada em escopo limitado, logger reprovado e build bloqueado. Esses resultados não foram executados novamente nesta etapa.

## Aceitação ainda impossível

Testes com limites de jitter/latência, qualidade analógica, protocolo definitivo e política segura ficam pendentes nos IDs DEC correspondentes. É permitido testar mecanismos com limites sintéticos claramente rotulados; proibido usar isso para aprovar o valor de produto ainda indefinido. Nenhum gate físico pode ser marcado por aprovação documental.
