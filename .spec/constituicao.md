# Constituição mecânica do SDD — v1.0.0

Este arquivo traduz para o motor onp-spec apenas as regras auditáveis da documentação. A constituição normativa do produto permanece em docs/constitution.md.

## P-001 [DEVE] Toda entrega auditada tem prova executável

Uma feature só recebe status auditada quando todos os critérios de aceite têm teste aprovado e o audit em modo CI termina sem erro.

- verificação(gate): intrínseca ao audit

## P-002 [DEVE] Existe uma única especificação normativa do produto

Os requisitos REQ-F e REQ-NF permanecem em docs/spec.md. Features mecânicas podem referenciá-los, mas não criar uma segunda cópia normativa.

- verificação(teste): @principle:P-002

## P-003 [DEVE] Estado documental não é estado de implementação

O histórico do SDD distingue conteúdo especificado, código implementado e evidência de verificação executada.

- verificação(teste): @principle:P-003

## P-004 [DEVE] Toda revisão do SDD é rastreável

A versão vigente e cada revisão publicada informam data, natureza da mudança, artefatos afetados e evidência disponível.

- verificação(teste): @principle:P-004
