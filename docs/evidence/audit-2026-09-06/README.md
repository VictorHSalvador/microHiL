# Evidência da avaliação HOST de 06.09.2026

Status: execução real e limitada de auditoria; não aprova o MICROHIL nem requisitos do Demo. Responsável pela execução: Codex nesta sessão; revisão humana pendente.

## Ambiente e identidade

- Diretório: `/home/linuxvh/Projects/microHiL`.
- HEAD: `c788a4283cf81f17e9a2956ae258487c7931590a`; fontes de produção sem modificações locais na inspeção inicial.
- Ubuntu 22.04.5 LTS, x86_64; kernel `6.8.0-138-generic`, PREEMPT_DYNAMIC.
- GCC 11.4.0; CMake 3.22.1.
- Sem Raspberry Pi/ESP32 conectado a este procedimento; sem instrumento elétrico ou medição RT.
- SHA256 DOCX REQ: `3611782ba23fba1ddc1a7c3a4b10715d2947f0210b04403582653ee52297f506`.
- SHA256 FMU: `b30de9ac780fdbf7eb4c4c8c913cbc72c30c2a9522c51807a4156cb2d535478c`.
- SHA256 logger: `8a11697ca8ba875acf5e3ff6c81201cdf1e3fd50fc591361b4f03d5b64150986`.
- SHA256 fila: `eb1b075fb790afd064ef7bf3bc9c33999aa28cdfcd301e2b26964a42e704e6e3`.
- SHA256 probe: `af4e97cb8c7d9e43116b85564d06c0cd1a2ea5081795f9d1b029d4c0f4e4d6d3`.

## EV-AUD-01 — Configuração limpa

Executado a partir da raiz do projeto:

```sh
cmake -S . -B /tmp/microhil-audit-build-20260906 -DCMAKE_BUILD_TYPE=Release
```

Esperado: dependências resolvidas e geração concluída. Observado, saída 1:

```text
-- The C compiler identification is GNU 11.4.0
CMake Error at CMakeLists.txt:20 (message):
  FMI Library not found. Set -DFMILIB_ROOT=/path/to/fmilib/install
-- Configuring incomplete, errors occurred!
```

Resultado: build integral bloqueado por dependência não encontrada pelo CMake no ambiente consultado. Não prova defeito de compilação no restante das fontes. O cache preexistente aponta para `/home/carla/.local/fmilib` e `/home/carla/Downloads/fmu_rt_runner`; não foi reaproveitado. Não foram instaladas dependências.

## EV-AUD-02 — Fila e falha de escrita do logger

Procedimento em [host_probe.c](host_probe.c): testar fila vazia, encher 4095 posições, rejeitar uma amostra excedente e drenar em ordem; repetir três vezes, verificando wrap e contador. Em seguida enfileirar uma amostra e executar o logger real com `/dev/full` e produção concluída. No Linux, esse dispositivo permite abrir o arquivo e falha nas escritas por falta de espaço, exercitando o tratamento de erro.

```sh
gcc -std=gnu11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -I include \
  docs/evidence/audit-2026-09-06/host_probe.c \
  src/app_config.c src/sample_queue.c src/csv_logger.c \
  -pthread -o /tmp/microhil-audit-host-probe
/tmp/microhil-audit-host-probe
```

Compilação observada: sucesso, sem diagnóstico do compilador. Saída da execução:

```text
QUEUE: PASS (empty, FIFO, 4095 usable slots, wrap, 3 drops)
sizeof(SimulationSample)=544 sizeof(SampleQueue)=2228248
LOGGER /dev/full: expected nonzero error; observed 0 => FAIL (write failure hidden)
```

Exit code do probe: **1**, indicando a falha detectada. O programa não transforma a reprodução do defeito em aprovação do logger.

Resultado fila: aprovado somente para casos sequenciais executados. Não houve stress entre produtor/consumidor, análise formal, sanitizers ou prova de atomics lock-free no alvo. Resultado logger: **reprovado** para propagação de falha de escrita, relevante a REQ-F-09/22. Não executa o main; o descarte de erro pelo main foi identificado por inspeção separada.

## EV-AUD-03 — Inspeção da FMU

Leitura de `modelDescription.xml` dentro do ZIP com Python/lxml; leitura dos primeiros 20 bytes do binário, sem execução. XML: FMI 2.0, CoSimulation, gerado por OpenModelica; `canHandleVariableCommunicationStepSize=true`. Contagem por `causality`: zero inputs, 14 outputs. Único binário: `binaries/linux64/ClosedLoopHiL_WGS84.so`, ELF class 2, little-endian, `e_machine=62` (x86-64).

Resultado: metadados e arquitetura identificados. Não valida comportamento matemático, dependências dinâmicas, execução FMI ou tempo real. Não há binário ARM incluído no arquivo inspecionado.

## Não executado

Build/link integral, execução da FMU, GUI/gnuplot, ROS 2, micro-ROS, USB, firmware, bancada, HIL, jitter/latência no alvo, persistência binária e reprodução dinâmica da corrida de encerramento. Os achados estáticos do relatório são identificados como tais. Nenhum dado do Demo foi usado como evidência real.
