# TASK-001 Build reproduzível e verificação HOST

Status: ficha preparada; implementação não iniciada e aguardando fechamento dos esclarecimentos/documentos conforme pedido do usuário. Requisitos: REQ-NF-01, REQ-NF-11, REQ-NF-13 e organização futura REQ-NF-18. Gates: G-DOC-HOST e G-HOST.

## Objetivo e ponto de partida

Permitir verificar o código aproveitado a partir de fontes e dependências identificadas, sem confiar no `build/` importado de outra máquina. O CMake atual exige FMILibrary antes de permitir qualquer alvo; a auditoria registrou falha de configuração e compilou fila/logger separadamente.

## Trabalho

1. Identificar instalação/origem/versão de FMILibrary e escolher revisão reproduzível compatível; registrar em TARGET/instruções de build. Não afirmar que um ramo móvel é lockfile.
2. Preparar targets de componentes HOST que não precisam de FMILibrary, mantendo falha explícita quando o runner completo for solicitado sem a dependência. Não declarar build parcial como integral.
3. Registrar teste de fila existente como ponto de partida e criar o teste de regressão de logger em TASK-002. O probe histórico é imutável; teste novo pode seguir estilo vigente.
4. Compilar em diretório limpo, verificar warnings e registrar compilador, opções e versões. Não reutilizar objetos preexistentes.
5. Executar testes descobertos pelo sistema de build e registrar quantos foram executados, não apenas exit code do runner de testes.

## Aceitação

- Núcleo de componentes HOST compila e testes são descobertos sem ROS/ESP32.
- Runner integral configura e compila com FMILibrary identificada, ou a subetapa permanece explicitamente bloqueada com log; não concluir TASK-001 inteira nesse caso.
- Fontes e headers próprios usam os avisos existentes sem ocultar diagnósticos para “passar”.
- Procedimento reproduzível e registro de versão/resultado em evidence; matriz atualizada.

## Fora de escopo

Escolher transporte, configurar pinos, instalar firmware, trocar GUI, resolver estado seguro ou declarar desempenho no Raspberry Pi. Estilo aprovado se aplica a código novo; migração global continua TASK-005. Não executar um modelo por minutos somente para provar que o executável foi compilado.
