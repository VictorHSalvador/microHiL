# EV-HOST-BUILD-2026-09-07 — Fundação de build HOST

Data de execução: 07.09.2026. Diretório de trabalho: `/home/linuxvh/Projects/microHiL`. Ambiente observado: Ubuntu 22.04, GCC 11.4.0 e CMake disponível no host. Cada configuração usou diretório temporário limpo em `/tmp`.

## Procedimentos e resultados observados

| Caso | Procedimento | Resultado observado |
|---|---|---|
| Build independente | `cmake -S . -B <tmp> -DMICROHIL_BUILD_RUNNER=OFF -DBUILD_TESTING=ON`, seguido de build | Configuração e build concluídos. As fontes próprias compilaram com `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`, sem warnings no output observado. |
| Descoberta e execução CTest | `ctest -N` e `ctest --output-on-failure` no diretório independente | Descobertos exatamente `sample_queue_spsc_order`, `sample_queue_spsc_wrap` e `sample_queue_spsc_saturation`; 3/3 executaram com sucesso. |
| Agregador documental | `node test/run_spec_tests.js` | 17/17 testes com sucesso, incluindo AC-007 a AC-014 e os princípios aplicáveis cobertos pelo agregador. |
| Dependência ausente | Runner solicitado sem FMILibrary disponível | A configuração falhou com diagnóstico das três rotas: `FMILIB_ROOT`, include/biblioteca explícitos ou `MICROHIL_FETCH_FMILIB=ON`. |
| Origem oficial | `MICROHIL_FETCH_FMILIB=ON`, seguido de build de `fmu_rt_runner` | Configuração e build concluídos com FMILibrary 3.0.4, revisão imutável `4a4b21ec10a632b2768a604c2330c54204919644`, de `https://github.com/modelon-community/fmi-library.git`. |
| Instalação externa | Include `/home/linuxvh/Projects/asturian-software/dev/external/include` e biblioteca `/home/linuxvh/Projects/asturian-software/release/external/lib/libfmilib.a` | `fmu_rt_runner` configurou e compilou sem download. A versão do artefato externo não foi comprovada e não é baseline. |

O build com a origem oficial mostrou warnings de headers/fontes de terceiros da FMILibrary. Eles não foram ocultados; esta evidência não os atribui às fontes próprias nem os trata como aprovação de qualidade da dependência externa.

## Vínculo aos critérios de aceite

| Critério | Evidência desta execução | Limite preservado |
|---|---|---|
| AC-007 | Configuração independente concluída em diretório limpo. | Não configura o runner FMI. |
| AC-008 | Componentes próprios compilados com os cinco avisos do projeto, sem warnings observados. | Não qualifica fontes de terceiros. |
| AC-009 | CTest descobriu e executou ordem, wrap e saturação, 3/3 com sucesso. | Não prova concorrência real SPSC. |
| AC-010 | Diagnóstico da ausência da dependência apresentou as três formas suportadas. | Não exerce uma FMU. |
| AC-011 | Runner compilado por fetch da origem oficial 3.0.4 na revisão fixa indicada. | Build não prova comportamento FMI. |
| AC-012 | Runner compilado com include/biblioteca explícitos sem download. | Versão e compatibilidade do artefato externo não comprovadas. |
| AC-013 | Ambiente, comandos, opções, dependência, quantidade de testes e limites estão registrados aqui e no README. | A repetição exige ambiente/dependências compatíveis. |
| AC-014 | Plano, tarefa, matriz, gates e baseline 0.7.2 foram atualizados somente no alcance demonstrado. | Não promove build HOST a validação de produto. |

## Limitações

Nenhuma FMU foi executada. Não houve validação de Raspberry Pi, DAQC, USB, ROS, GUI, eletricidade, concorrência real SPSC ou deadlines. Os resultados não qualificam o target físico nem medem tempo real.

## Fechamento mecânico

`onp-spec verify host-build-foundation` comprovou 8/8 critérios de aceite com PASS a partir de 17 testes lidos. `onp-spec verify sdd-versioning` comprovou 6/6 critérios da governança. A execução final de `onp-spec audit --ci` retornou 14/14 critérios com teste, 14/14 provados e zero avisos.
