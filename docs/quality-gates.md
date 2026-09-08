# Gates de qualidade por incremento

Status: G-CONSOLIDACAO aprovado em 07.09.2026; liberação de produto não realizada. Gates se aplicam ao escopo escolhido, não a todos os requisitos de todas as etapas simultaneamente.

## G-CONSOLIDACAO — Condição atual para iniciar código

- [x] Respostas Q incorporadas sem ambiguidades que afetem o desenvolvimento.
- [x] Requisitos/arquitetura/ICD/TARGET/verificação concordam com as decisões confirmadas.
- [x] Fase documental concluída; nenhuma proposta pendente foi tratada como resposta.

Esse gate reflete a instrução específica do usuário de só começar desenvolvimento após sanear os Markdown. A aprovação se apoia na [revisão da política de perda](evidence/realtime-loss-policy-review-2026-09-07.md) e na [prova mecânica do versionamento](../.spec/verification/sdd-versioning.json). Ela libera a implementação incremental prevista no plano, sem aprovar os gates HOST, ICD/TARGET ou TIMING/HIL. Parâmetros explicitamente atribuídos à implementação, seleção de versões ou medição continuam sob os gates correspondentes.

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
- [ ] Layout, versão de perfil, sequência, timeout, estados e descarte sem retransmissão completos.
- [ ] SDK/toolchain/firmware e host identificados, incluindo domínio de relógios.
- [ ] Política elétrica de AO/DO/PWM especificada separadamente da retenção de inputs no host; procedimento de bancada aprovado.
- [ ] READ_ACK e buffers limitados definidos; DISABLE em 60 s sem progresso, sem presumir observação direta do endpoint pelo ESP32.

## G-TIMING-HIL — Antes de alegar desempenho integrado

- [ ] Passo, jitter, deadline, timeout, duração/carga de teste e limites aprovados.
- [ ] Medições no alvo e dados brutos identificam scheduler, configuração e instrumentos.
- [ ] Malha, saturação, dados inválidos/antigos, gaps, reset e desconexão exercitados; nenhum teste espera replay de DATA.
- [ ] Interferência da supervisão/comunicação entre os dois núcleos medida sob carga e fila cheia, sem bloqueio ilimitado na aquisição/atuação.
- [ ] Limitações e resultados reprovados preservados; nenhum dado fictício substitui ensaio.

Após G-CONSOLIDACAO, falha de um gate específico mantém aberta a tarefa dependente sem bloquear automaticamente incrementos independentes. “Gates definidos” e “gates aprovados” são estados diferentes.
