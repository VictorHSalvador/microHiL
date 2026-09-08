# Spec: Fundação de build HOST

> feature: host-build-foundation
> status: pronta

## Contexto

O protótipo só configura quando encontra uma FMILibrary instalada e o diretório `build/` versionado aponta para caminhos de outra máquina. Este incremento torna o build HOST verificável a partir de diretório limpo, permite testar componentes independentes sem FMILibrary e mantém a falha explícita quando o runner completo é solicitado sem sua dependência. Ele executa a TASK-001 do plano e atende REQ-NF-01, REQ-NF-11, REQ-NF-13 e a preparação modular de REQ-NF-18.

## Histórias

### US-004 — Verificar componentes HOST sem dependências de integração

Como desenvolvedor do MICROHIL, quero configurar e testar os componentes HOST independentes da FMILibrary, para detectar regressões de infraestrutura mesmo quando a integração FMI ainda não está instalada.

#### AC-007 — O build independente configura em diretório limpo

- **Dado** um host com CMake, compilador C11 e pthreads, sem informar FMILibrary
- **Quando** o projeto é configurado com o runner FMI desabilitado e testes habilitados
- **Então** a configuração termina com sucesso e gera os alvos HOST independentes

#### AC-008 — Os componentes independentes compilam com os avisos do projeto

- **Dado** o build independente configurado
- **Quando** seus alvos são compilados
- **Então** as fontes próprias compilam com `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`, sem ocultar avisos para obter sucesso

#### AC-009 — Os testes HOST são descobertos e executados

- **Dado** o build independente concluído
- **Quando** o CTest enumera e executa os testes
- **Então** encontra testes nomeados para a fila SPSC e todos terminam com sucesso, cobrindo ordem, wrap e saturação

### US-005 — Diagnosticar e reproduzir a integração FMILibrary

Como desenvolvedor do runner, quero uma seleção explícita e rastreável da FMILibrary, para compilar a integração FMI sem reutilizar caminhos absolutos ou artefatos anônimos de outra máquina.

#### AC-010 — A ausência da dependência é informada somente quando necessária

- **Dado** um host sem FMILibrary informada ou instalada
- **Quando** o runner FMI é solicitado
- **Então** a configuração falha com diagnóstico que informa as formas suportadas de fornecer ou obter a dependência

#### AC-011 — A versão oficial selecionada é imutável para o build

- **Dado** acesso à origem oficial da FMILibrary
- **Quando** a obtenção automática é habilitada
- **Então** o build usa a release 3.0.4 identificada por revisão fixa, sem seguir branch móvel, e consegue compilar o runner completo

#### AC-012 — Uma instalação explícita continua suportada

- **Dado** headers e biblioteca FMILibrary compatíveis fornecidos pelo integrador
- **Quando** seus caminhos são passados explicitamente ao CMake
- **Então** o runner configura e compila sem baixar outra cópia, registrando que a versão da instalação externa deve ser comprovada pelo integrador

### US-006 — Preservar evidência reproduzível do incremento

Como responsável técnico, quero registrar comandos, ambiente, resultados e limites da TASK-001, para distinguir build independente, build integrado e validação de produto.

#### AC-013 — A documentação permite repetir os builds

- **Dado** a conclusão dos testes da fundação HOST
- **Quando** o procedimento e a evidência são consultados
- **Então** eles informam opções CMake, comandos limpos, ferramentas, versão da dependência, quantidade de testes e limitações

#### AC-014 — O SDD reflete o estado observado

- **Dado** os resultados executados da TASK-001
- **Quando** plano, matriz, gates e registro de versão são atualizados
- **Então** somente os requisitos e subetapas demonstrados avançam de estado e nenhum build HOST é apresentado como validação da FMU, do Raspberry Pi, da DAQC ou de deadlines

## Fora de escopo

- Alterar o comportamento do ciclo de simulação, logger, formato binário, GUI, comunicação USB/ROS ou firmware.
- Validar execução de uma FMU, propriedades elétricas, Raspberry Pi ou desempenho temporal.
- Migrar globalmente a nomenclatura do código legado; somente código novo segue a constituição nesta feature.

## Suposições

Nenhuma. A FMILibrary 3.0.4 foi selecionada a partir da release oficial e sua compatibilidade declarada com FMI 2.0 será verificada pelo build, não tratada como resultado antecipado.

## Perguntas em aberto

Nenhuma. A origem e a versão da dependência são decisões técnicas reversíveis desta tarefa e não alteram os requisitos do produto.
