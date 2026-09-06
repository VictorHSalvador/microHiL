# EV-ICD-RASPDAQ — Inspeção e adaptação da referência

Data: 06.09.2026. Revisão documental 0.6. Método: leitura estática das fontes locais, consultas primárias, construção independente de vetores de layout com Python struct e validação de Markdown. Não executados/importados: módulos RaspDAQ, serviços systemd, configuração USB, agente ROS, FMU ou firmware. Nenhuma escrita na referência externa.

## Origem observada

Referência OT1-HiLInfrastructure HEAD 64231dbc905217dc45e753f8540867277a34bafe, git status limpo no momento da inspeção. MicroHiL mantém código auditado c788a4283cf81f17e9a2956ae258487c7931590a, com commits/documentos posteriores e alterações locais de documentação; nenhuma mudança em src/include/CMakeLists nesta revisão.

| Fonte lida | SHA-256 |
|---|---|
| [raspdaq_ffs.py](/home/linuxvh/Projects/OT1-HiLInfrastructure/RaspDAQ/RaspDAQ-root/raspdaq_ffs.py) | `a32fe31e49003c9370b9ee6026bc6b3a5555bca321fcc86aa72ffb621208ecea` |
| [raspdaq_main.py](/home/linuxvh/Projects/OT1-HiLInfrastructure/RaspDAQ/RaspDAQ-root/raspdaq_main.py) | `7c7e91efdf55fed290e1d4c7d134e9523d5e834db790259b1412c663aacbbf67` |
| [run_raspdaq_ffs.sh](/home/linuxvh/Projects/OT1-HiLInfrastructure/RaspDAQ/RaspDAQ-root/run_raspdaq_ffs.sh) | `32c13f1f94a2fd63c0966a63ada9b4549ba1ac0bf4880008dbd97741d42bd057` |
| [raspdaq-ffs.service](/home/linuxvh/Projects/OT1-HiLInfrastructure/RaspDAQ/RaspDAQ-systemd/raspdaq-ffs.service) | `e0a10dddca13d82aec7fa22a51c757120be3563b7a8c1b0011b79f83f96ef71e` |
| [shared_daq_state.py](/home/linuxvh/Projects/OT1-HiLInfrastructure/ros2_ws/src/raspdaq_bridge/raspdaq_bridge/shared_daq_state.py) | `ea14ef68a1e834f40c97eb76aa81e64fb5bae7e50d06bfe98b3b8c385ffc785b` |
| [raspdaq_node.py](/home/linuxvh/Projects/OT1-HiLInfrastructure/ros2_ws/src/raspdaq_bridge/raspdaq_bridge/raspdaq_node.py) | `18fa4e1467ddb90267b7bc8873a98c838fe9a82b3314493ad36abfe992c43022` |
| [LinuxHost_node.py](/home/linuxvh/Projects/OT1-HiLInfrastructure/ros2_ws/src/raspdaq_bridge/raspdaq_bridge/LinuxHost_node.py) | `4abfffa29831728d80e48b734b66d61e6639b7c33db16a884f05fd14ed8d109f` |
| [8raspTest-MID-SEQ_Ciclic-Print.py](/home/linuxvh/Projects/OT1-HiLInfrastructure/HostMachine/WindowsHost/Sandbox/USB_Communication/8raspTest-MID-SEQ_Ciclic-Print.py) | `71a7c30e87f17676151c1b1cf1dde8917aa6754155b9dae7b75ad3d9c05204ab` |

## Achados que fundamentam o ICD

- raspdaq_ffs.py: constantes CONFIG 4/5, DATA 256 e payload 251; struct `<HB`/`<HBH`; FieldSpec/PayloadSchema com ordem fixa; zeros nos bytes reservados. Código define SYNC AA55, MIDs 01/02/03 e comandos 00/01/02/03. Esses valores MICROHIL não copia, preservando decisões do usuário.
- classify_seq usa delta uint16, meia faixa 0x8000 e categorias primeiro/repetido/esperado/salto/antigo. Não há SID explícito no frame. SEQ não é prova de idade física nem ACK cumulativo.
- _process_usb_rx_packet e _usb_tx_loop ordenam CONFIG e DATA com usb_write_lock, rechecando streaming antes de escrever. _handle_data verifica estado antes de atualizar snapshot. Locks e write_full podem bloquear; não comprovam limites de tempo do MICROHIL.
- handle_hil_to_daq filtra não finitos por campo preservando anterior e devolvendo nomes inválidos. MICROHIL adapta para aquisição no host e mantém qualidade visível ao contador por passo.
- shared_daq_state.py: instância persistente, RLock, snapshots frozen/slots, geração e timestamp local; clear_all limpa sessão sob lock. Cache zerado em memória não é medição de saída física.
- raspdaq_main integra USB e nó ROS num processo; LinuxHost_node é loopback ROS. Não são cliente/agente micro-ROS do ESP32.
- CLI 8… é Sandbox, usa PyUSB/libusb-package, descobre endpoints Bulk e aceita STATUS MID04 além de CONFIG/DATA; o codec do serviço não implementa esse MID. CLI documenta ausência LEN/CRC e configura timeouts 200/1000 ms, incompatíveis com ≤5 ms aprovado no MICROHIL.
- Runtime TX configura 0,005 s e reajusta origem monotônica quando atrasado. Não prova 200 Hz e não substitui a grade fixa de simulação MICROHIL.
- Buscas em RaspDAQ, raspdaq_bridge e ferramentas USB referidas não encontraram protocolo de ACK cumulativo/sessão, transporte XRCE ou formato binário MICROHIL. A conclusão é limitada aos componentes relevantes inspecionados, não afirma inexistência em todo sistema externo.

## Verificações executadas e resultados

- Sete vetores base CONFIG/DATA comparados com Python struct independente: iguais aos bytes documentados. Isso é verificação aritmética do layout, não teste do encoder de produto nem interoperabilidade.
- Base DATA tem 5+N bytes; N=256 implica 261 bytes totais. Não aplicar máximo 256 ao frame inteiro.
- 59 requisitos e 59 linhas correspondentes de rastreabilidade; critérios V/design/tarefas/pendências presentes. Links locais resolvidos. Referências originais comparadas byte a byte com Downloads, sem mudanças. git diff --check sem erros; código de produção preservado.

## Limitações e continuidade

A revisão responde item a item em ADR-003, separando o que RaspDAQ resolve do que não fornece. A base não implementa E-SESSION/E-ACK/E-INTEGRITY/E-TIME/E-XRCE; propostas documentadas não encerram comprovação de hardware. IF-LOG é design próprio derivado dos requisitos, não formato encontrado na referência. G-CONSOLIDACAO permanece aberto para versão integrada; não há declaração de aprovação de produto.

Fontes técnicas complementares: [micro-ROS custom transports](https://github.com/micro-ROS/micro-ros.github.io/blob/master/_docs/tutorials/advanced/create_custom_transports/index.md) confirma callbacks nos dois lados, e [CH341 Linux v6.8](https://raw.githubusercontent.com/torvalds/linux/v6.8/drivers/usb/serial/ch341.c) identifica necessidade de configuração específica da ponte. Não foi executado esse driver nem escolhida baud rate por inferência.
