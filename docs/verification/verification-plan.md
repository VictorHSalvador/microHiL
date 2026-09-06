# Plano de verificação

Revisão 0.6: procedimentos propostos para 59 requisitos; sem ensaio novo de produto nesta revisão. Implementação e execução desses testes aguardam fechamento documental solicitado. Cada `V-F-*`/`V-NF-*` em [spec.md](../spec.md) é um caso planejado associado ao mesmo número de requisito; [traceability.md](../traceability.md) identifica design, tarefa e evidência real disponível.

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
- **Tempo:** clock/sleep injetáveis para verificar fórmulas, atraso antes/depois do sleep, continuidade após overrun, pior perda, último passo e Stop. Ensaiar referência 100 Hz/10 ms e timeout máximo 5 ms por transferência; separar configuração de tempo observado. Em alvo, medir sob carga definida de CPU, comunicação, disco e GUI; registrar scheduler/afinidade efetivos, duração e configuração. Medidas lógicas e elétricas devem ter referências explicitadas; médias e máximos não substituem limites de aceitação pendentes.
- **Interface:** vetores de referência de tipos/byte order/layout após DEC-012; fragmentação/agregação, ruído, tamanho inválido, frame parcial, timeout, sequência repetida/antiga/wrap, sessão e versão incompatível. Bulk/libusb e micro-ROS estão mantidos: testar ambos e a arquitetura XRCE definida em Q-01. Vetores do protocolo usam SYNC 0x7259; não usar BB77 como valor vigente. Testar payload 256 em frame maior, STATUS uint8 com códigos COMMAND apenas em CONFIG conforme ADR-003, payload contendo SYNC e DISABLE sob streaming.
- **GUI/persistência:** interação durante execução, janelas por variável, limites Y, atualização independente de no máximo 10 Hz, desligar atualização sem parar o núcleo, salvar/carregar perfil, versão/corrupção e integridade de log. Conferir ticks −2…2 de 1 em 1, janela comum deslizante, abertura antes de Play e histórico destruído ao fechar/reabrir. Formato binário e CSV posterior confirmados; Falha de log mantém execução com aviso; Q-07 fecha representação tipada/metadados.
- **Firmware/bancada:** identificar circuito/instrumento e estimular canais dentro do perfil; medir calibração, atuação, boot/reset, watchdog, falta de dados, link e recuperação. Separar contrato de atuação física de F-23/F-24 no host; F-27 não exige zeramento. Taxas, limites de erro, duração/repetições precisam ser aprovados antes do ensaio de aceitação.
- **Aquisição/proteção (V-F-23/24):** DAQC válido A → NaN/Inf/inválido → válido B; inputs FMU recebem A retido até B. Sem histórico, usam inicial válido do input; não fabricar zero. Testar 99/100 passos por canal, vários pacotes por passo, vários canais atingindo limite, checkbox ligado/desligado e novo Play. Valor constante válido quebra sequência; indicar canal adquirido/input FMU, não output de atuação. Não zerar atuadores como efeito dessa proteção. Verificar candidato selecionado na fronteira do passo e separar ausência de pacote.
- **Streaming sem leitura (V-F-27):** parar consumidor Linux mantendo host e outros serviços vivos; testar 60 s sem avanço de ACK de leitura desde STREAMING e após progresso anterior. ACK deve avançar por DATA efetivamente lido, inclusive com NaN; heartbeat/ACK duplicado/sessão antiga não renovam. Não desabilitar antes de 60 s; medir latência da transição após limiar e carga de buffers. CONFIG deve continuar responsivo; nenhum zeramento é requisito. Validar reinício sem backlog da sessão anterior.
- **Conversão:** ler binário tipado com FMU correta e incorreta, seleção/ordem diferente e truncamento; converter apenas após encerramento e comparar amostras finais por passo. Não exigir métricas por passo no arquivo.
- **Estilo:** inspeção de símbolos próprios C/Python, comentários e quebras de linha, com exceções de ABI/terceiros. Revisão de comentário exige leitura humana/técnica; regex não determina se a frase é útil ou “comentário de IA”.

## Evidência mínima

ID de teste e requisito, commit e alterações locais, hash de FMU/firmware/fixture, versões, SO/arquitetura, perfil de placa, configuração, instrumentos/calibração aplicável, comando/procedimento, dados brutos, cálculo, esperado, observado, resultado, limitações e responsável pela execução. Aprovação de produto é distinta de execução do teste.

Reutilizar a pasta `docs/evidence/` por execução. Nenhum resultado planejado recebe arquivo EV com números inventados. A [auditoria inicial](../evidence/audit-2026-09-06/README.md) contém fila sequencial aprovada em escopo limitado, logger reprovado e build bloqueado. Esses resultados não foram executados novamente nesta etapa.

## Aceitação ainda impossível

Testes com limites de jitter/latência, qualidade analógica, protocolo definitivo e política segura ficam pendentes nos IDs DEC correspondentes. É permitido testar mecanismos com limites sintéticos claramente rotulados; proibido usar isso para aprovar o valor de produto ainda indefinido. Nenhum gate físico pode ser marcado por aprovação documental.

Verificações adicionais planejadas: múltiplos pacotes inválidos no mesmo passo não antecipam 100 passos; inválidos em canais distintos não somam contador global. Supervisão em núcleo separado com filas saturadas deve ter custo e interferência medidos na aquisição/atuação; host não espera ACK dentro de doStep. SCHED_FIFO negado deve produzir diagnóstico. Propostas de tolerância, agenda e seleção temporal precisam ser consolidadas antes de declarar aceitação.

## Casos adicionais da revisão 0.5

- V-F-02/NF-16: opções ADC/LEDC do TARGET, pares frequência/resolução impossíveis, timers compartilhados e GPIO conflitante; configuração inválida não aplica parcialmente. Calibração e frequência física exigem bancada.
- V-F-07: fim/Stop/Error → zero nos atuadores e fim de DATA; nova execução aplica outputs iniciais do modelo reinicializado. DATA antigo não sobrescreve zero. Separar essa regra de F-27 e da retenção dos inputs.
- V-F-23/24: várias atualizações no passo, última antes da inserção determina candidato; thread USB independente. Sem pacote incrementa timeout, não invalidade numérica. Conferir contador antes/depois de intervalo sem pacotes.
- V-NF-28: passo 10 ms, conclusão em 12 ms → próxima liberação 20 ms; um deadline perdido, uma liberação não utilizada, atraso máximo 2 ms. Testar limites exatos, várias liberações perdidas, sequência/h FMI inalterados e ausência de compensação. Resultados somente após execução; duração de parede pode superar duração do modelo.

## Verificação da adaptação RaspDAQ a planejar

Vetores base: CONFIG 4/5 bytes, SYNC 59 72, DATA 5+N (até 261 bytes), seq 65535→0, duplicado/antigo/gap e schema por direção. Parser UART deve dividir cada quadro em todas as posições possíveis e consumir vários quadros em um read; a referência não demonstra isso. Testar ACK de STREAMING antes do primeiro DATA e ausência de DATA após confirmação de parada, sem mutex sob I/O ilimitado.

Extensões precisam de vetores próprios e versão incompatível explicitamente rejeitada: sessão antiga, wrap ambíguo, ACK atrasado, CRC incorreto, micro-ROS sob saturação e CH340 com timeout. Comparar binário/CSV pelo layout IF-LOG usando tipos e nomes explícitos; não depender do CLI RaspDAQ como oracle desses formatos inexistentes nele.
