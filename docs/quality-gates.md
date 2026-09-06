# Gates de qualidade por incremento

Status inicial: critérios definidos; liberação de produto não realizada. A verificação documental da sessão será registrada separadamente. Gates se aplicam ao escopo escolhido, não a todos os requisitos de todas as etapas simultaneamente.

## G-CONSOLIDACAO — Condição atual para iniciar código

- [ ] Respostas Q incorporadas sem ambiguidades que afetem o desenvolvimento.
- [ ] Requisitos/arquitetura/ICD/TARGET/verificação concordam com as decisões confirmadas.
- [ ] Fase documental concluída; nenhuma proposta pendente foi tratada como resposta.

Esse gate reflete a instrução específica do usuário de só começar desenvolvimento após sanear os Markdown. Independência técnica de um módulo HOST não revoga essa ordem.

## G-DOC-HOST — Antes de implementação independente do hardware

- [ ] Requisitos do incremento identificados e comportamento/testes definidos.
- [ ] Dependências e decisões que afetam esse incremento resolvidas ou excluídas explicitamente do escopo.
- [ ] Código a reaproveitar, erros conhecidos e limitações de evidência identificados.
- [ ] Regras de estilo vigentes e interfaces afetadas compreendidas.

Ausência de pinagem não reprova uma correção isolada de logger. Campo de formato ou política em aberto pode bloquear sua integração operacional, sem impedir testar propagação de erro.

## G-HOST — Antes de concluir tarefa de software no host

- [ ] Build limpo dos alvos declarados, com versões e warnings revisados.
- [ ] Testes de comportamento e falha executados; nenhum “zero testes” apresentado como suíte aprovada.
- [ ] Resultado agregado e integridade de dados coerentes, incluindo falhas.
- [ ] Comentários/nomenclatura do escopo atendem ADR-001; APIs externas preservadas.
- [ ] Evidência e matriz atualizadas, sem apresentar teste parcial como produto validado.

## G-ICD-TARGET — Antes de driver definitivo e atuação

- [ ] DEC de transporte resolvida e requisitos afetados revisados.
- [ ] Perfil físico, pinos, circuitos, unidades, faixas e cargas confirmados.
- [ ] Layout, versão, sessão, sequência, timeout, estados e erro completos.
- [ ] SDK/toolchain/firmware e host identificados, incluindo domínio de relógios.
- [ ] Estado seguro por canal e procedimento de ensaio aprovados para a bancada.

## G-TIMING-HIL — Antes de alegar desempenho integrado

- [ ] Passo, jitter, deadline, timeout, duração/carga de teste e limites aprovados.
- [ ] Medições no alvo e dados brutos identificam scheduler, configuração e instrumentos.
- [ ] Malha, saturação, dados inválidos/antigos, reset, desconexão e recuperação exercitados.
- [ ] Limitações e resultados reprovados preservados; nenhum dado fictício substitui ensaio.

Após G-CONSOLIDACAO, falha de um gate específico mantém aberta a tarefa dependente sem bloquear automaticamente incrementos independentes. “Gates definidos” e “gates aprovados” são estados diferentes.
