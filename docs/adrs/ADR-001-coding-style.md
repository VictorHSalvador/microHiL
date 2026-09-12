# ADR-001 Convenções de código e comentários

Status: **confirmada por instrução explícita do usuário**, 06.09.2026. Escopo: código próprio C e Python; comentários e quebra de linha de todo código próprio.

## Contexto

REQ-NF-26 recebido prescrevia variáveis camelCase e funções UPPER_CASE. O protótipo usa funções snake_case e tipos PascalCase. O usuário substituiu explicitamente a convenção: funções C PascalCase, variáveis snake_case, constantes UPPER_CASE e tipos próprios snake_case com `_t`. Também definiu Python conforme tabela PEP 8 de nomes, comentários de desenvolvedor em português ou inglês e ausência de quebras cosméticas de parâmetros.

## Decisão

Aplicar [constitution.md](../constitution.md) e revisar a redação de REQ-NF-26/27 em [spec.md](../spec.md), conservando IDs e origem. Guardar a referência recebida sem modificação. Não pedir nova confirmação dessa substituição. A frase final truncada do pedido não gera lacuna para Python, pois a tabela anterior especifica funções snake_case.

## Consequências

Migrar nomes e comentários em incrementos que preservem build e comportamento, incluindo headers e chamadores. Não renomear APIs de terceiros ou `main`. Evidências históricas conservam o código efetivamente ensaiado. Não impor largura numérica que o usuário não definiu. Nomenclatura interna C++ será estabelecida antes da tarefa GUI; comentários em todo código próprio de produção devem ser breves, técnicos e em português ou inglês, sem explicação linha a linha ou quebra cosmética de chamadas.
