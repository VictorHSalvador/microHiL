# Spec: Governança e versionamento do SDD

> feature: sdd-versioning
> status: auditada

## Contexto

O repositório já possui especificação, arquitetura, ICD, decisões, plano, rastreabilidade e evidências, mas o histórico das revisões está distribuído entre vários arquivos. Esta feature cria um registro único para identificar a versão vigente e audita mecanicamente a coerência básica do conjunto. A fonte normativa dos requisitos do produto continua sendo docs/spec.md.

## Histórias

### US-001 — Consultar a versão vigente do SDD

Como integrante do projeto, quero localizar a versão documental vigente e seu estado, para saber quais decisões orientam o desenvolvimento sem confundir documentação com código validado.

#### AC-001 — A versão vigente é identificável

- **Dado** o índice documental do MICROHIL
- **Quando** uma pessoa procura a versão atual do SDD
- **Então** encontra um único arquivo de versionamento com número, data, status e links para os documentos normativos

#### AC-002 — O estado separa especificação, implementação e verificação

- **Dado** uma revisão que alterou somente Markdown
- **Quando** seu registro é consultado
- **Então** o documento informa separadamente o que foi especificado, implementado e efetivamente verificado

### US-002 — Rastrear alterações do SDD

Como desenvolvedor ou revisor, quero consultar as revisões do SDD em ordem cronológica, para relacionar mudanças a requisitos, decisões, arquivos e evidências.

#### AC-003 — Cada revisão possui metadados mínimos

- **Dado** o histórico desde a primeira consolidação
- **Quando** uma revisão é adicionada
- **Então** ela registra versão, data, natureza, resumo, artefatos afetados e evidência sem reescrever resultados históricos

#### AC-004 — O procedimento de atualização é explícito

- **Dado** uma futura alteração em requisito, arquitetura, contrato, tarefa, teste ou evidência
- **Quando** o responsável prepara a revisão
- **Então** encontra regras para escolher a versão, atualizar os artefatos relacionados e registrar o commit quando disponível

### US-003 — Detectar deriva documental básica

Como responsável técnico, quero uma checagem automática dos vínculos essenciais, para detectar requisitos sem linha de rastreabilidade, links locais quebrados e duplicação da autoridade normativa.

#### AC-005 — Requisitos e matriz permanecem alinhados

- **Dado** os requisitos em docs/spec.md e a matriz em docs/traceability.md
- **Quando** a verificação documental é executada
- **Então** os mesmos 59 IDs únicos aparecem nos dois artefatos e todos os links Markdown locais resolvem

#### AC-006 — A camada mecânica não duplica requisitos do produto

- **Dado** a estrutura .spec usada pelo motor
- **Quando** sua feature de governança é examinada
- **Então** ela aponta para docs/spec.md como fonte normativa e não redefine requisitos REQ-F ou REQ-NF

## Fora de escopo

- Implementar ou validar o runtime MICROHIL, firmware, GUI, comunicação, FMU ou hardware.
- Converter todos os 59 requisitos do produto para histórias US/AC duplicadas dentro de .spec.
- Alterar evidências históricas para refletir decisões posteriores.

## Suposições

Nenhuma.

## Perguntas em aberto

Nenhuma.
