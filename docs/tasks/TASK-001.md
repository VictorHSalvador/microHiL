# TASK-001 Build reproduzível e verificação HOST

Status: concluída, evidenciada e auditada em 07.09.2026. Requisitos: REQ-NF-01, REQ-NF-11, REQ-NF-13 e preparação modular de REQ-NF-18. Gates: G-DOC-HOST e G-HOST, somente nos itens demonstrados.

## Objetivo e ponto de partida

Permitir verificar o código aproveitado a partir de fontes e dependências identificadas, sem confiar no `build/` importado de outra máquina. O build independente separa os componentes HOST da dependência FMI; o runner completo mantém a dependência explícita e rastreável.

## Trabalho

1. A FMILibrary 3.0.4 foi obtida da origem oficial em revisão fixa `4a4b21ec10a632b2768a604c2330c54204919644`; a configuração e o build de `fmu_rt_runner` concluíram.
2. O build independente configurou e compilou com `MICROHIL_BUILD_RUNNER=OFF` e `BUILD_TESTING=ON`, sem procurar FMILibrary.
3. O CTest descobriu e executou `sample_queue_spsc_order`, `sample_queue_spsc_wrap` e `sample_queue_spsc_saturation`, com 3/3 sucessos. Esses testes sequenciais não demonstram concorrência real SPSC.
4. As fontes próprias foram observadas sob `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`, sem warnings no output observado. Warnings de headers/fontes de terceiros da FMILibrary foram preservados e registrados como limitação.
5. A rota de instalação explícita configurou e compilou `fmu_rt_runner` sem download com os caminhos externos registrados; a versão desse artefato não foi comprovada. A ausência de FMILibrary ao solicitar o runner produziu diagnóstico com `FMILIB_ROOT`, include/biblioteca explícitos e fetch da revisão fixa.

## Aceitação

- Núcleo de componentes HOST compilado e testes descobertos sem ROS/ESP32.
- Runner integral configurado e compilado com FMILibrary oficial identificada; instalação externa explícita também compilada sem download, sem constituir baseline.
- Fontes próprias compiladas com os avisos do projeto, sem ocultação observada; warnings da FMILibrary de terceiros não foram ocultados.
- Procedimento reproduzível, SDD 0.7.2, gates e matriz registrados na [evidência](../evidence/host-build-foundation-2026-09-07.md).

## Fora de escopo

Executar uma FMU, escolher transporte, configurar pinos, instalar firmware, trocar GUI, resolver estado seguro ou declarar desempenho no Raspberry Pi. Também ficaram fora de escopo DAQC, USB, ROS, eletricidade, concorrência real SPSC e deadlines. Estilo aprovado se aplica a código novo; migração global continua TASK-005.
