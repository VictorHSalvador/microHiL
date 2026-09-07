# EV-DOC-ICD-007 — Consolidação da política real-time do ICD

Data: 07.09.2026. Escopo: revisão documental 0.7 do MICROHIL. Executor: Codex nesta sessão, com autorização do usuário para aplicar a metodologia HiL. Nenhum código de produção, firmware, serviço, dispositivo USB, FMU ou nó ROS foi executado.

## Entradas

- Decisão explícita do usuário: não usar CRC; em dado ausente ou perdido, seguir para o próximo; preservar ao máximo a arquitetura RaspDAQ.
- Decisões anteriores confirmadas: thread USB separada, último snapshot antes da inserção FMI, timeout de transferência até 5 ms, confirmação cumulativa de leitura, DISABLE após 60 s sem progresso e micro-ROS no ESP32.
- Referência estática RaspDAQ registrada na evidência 0.6 e no ADR-003.

## Alterações verificadas

- DATA permanece little-endian, MID 02, SEQ uint16 e payload fixo N por direção, N até 256.
- DATA não possui CRC, NACK, retransmissão, replay, sessão no fio ou fila crescente.
- Repetido, antigo, incompleto e fora do prazo são descartados; salto de SEQ contabiliza lacuna e aceita o pacote novo.
- CONFIG continua separado e idempotente. Repetição de controle ocorre fora do núcleo e não recupera DATA.
- READ_ACK usa MID 03 e quadro SYNC[2], MID[1], SEQ[2]. Confirma consumo DAQC→host sem confirmar validade numérica ou pedir reenvio.
- XRCE usa MID 04 e compartilha o coordenador único do enlace; tráfego periódico é best effort.
- A configuração do perfil ocorre fora de STREAMING, por transação micro-ROS idempotente, e deve confirmar o hash do descritor antes de aceitar STREAMING.
- Entrada em STREAMING limpa fragmentos/mailboxes e reinicia SEQ/ACK. DATA fora de STREAMING é descartado.

## Verificações executadas

Uma leitura automática, sem modificar arquivos, contou 59 IDs únicos de requisito no spec e 59 linhas únicas na matriz, sem IDs ausentes ou excedentes. A resolução de links Markdown encontrou apenas o link para esta própria evidência antes de sua criação; após a criação ele deve ser repetido no fechamento do gate.

Vetores calculados com serialização little-endian independente:

| Vetor | Bytes observados |
|---|---|
| ENABLE host | 59 72 01 02 |
| Confirmação ENABLE | 59 72 01 02 02 |
| READ_ACK SEQ 7 | 59 72 03 07 00 |
| Cabeçalho DATA SEQ 7 | 59 72 02 07 00 |

git diff --check não apresentou erro. git diff --name-only restrito a src, include e CMakeLists.txt não retornou arquivo: código de produção permaneceu inalterado nesta revisão.

## Resultado e limites

Resultado documental: coerente para iniciar implementação incremental após a verificação final de links e referências. O contrato agora representa a decisão de baixa latência sem recuperação de amostras e separa CONFIG, DATA, READ_ACK e XRCE.

Não demonstrado: parser funcionando, ressincronização sob ruído, desempenho do CH340, custo do micro-ROS, alcance da cadência de ACK, integridade de um quadro sintaticamente válido com bits alterados, DISABLE em 60 s, zero físico, meta de 100 Hz ou comportamento elétrico. Sem CRC, um valor corrompido ainda plausível pode não ser detectado; esta é limitação aceita do contrato e deve permanecer nas evidências de bancada.
