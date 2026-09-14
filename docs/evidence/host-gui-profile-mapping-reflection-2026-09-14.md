# Reflexo de mapeamento YAML na GUI HOST — 14.09.2026

## Alteração

Após carregar um perfil YAML, a GUI recriava suas linhas de mapeamento com canal, escala e offset vazios. O perfil já era aplicado à sessão C, mas a apresentação não mostrava os GPIOs resolvidos. A sessão passou a expor cada `profile_mapping_t` carregado por API de leitura, e o controlador Qt o entrega ao QML. Ao reconstruir as linhas, o QML associa variável e direção ao mapa carregado; sinais sem entrada no YAML, como outputs de gráfico, continuam sem DAQC.

## Verificação observada

O teste `execution_session_lifecycle` confirma que a API devolve a primeira associação resolvida do perfil (`GPIO32_AI`, input) e rejeita índice fora do perfil. A variante ROS da GUI compilou e o CTest aprovou 49/49 casos; o loopback UDP foi ignorado por pré-condição de ambiente. A inspeção visual do preenchimento após novo carregamento do YAML continua pendente.

## Limites

A verificação HOST não abre diálogos nem mede I/O da DAQC. Ela não altera o conteúdo do YAML nem mapeia automaticamente sinais ausentes; somente reflete na GUI o perfil que foi validado e carregado pela sessão.
