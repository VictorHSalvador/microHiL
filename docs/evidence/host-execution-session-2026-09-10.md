# Evidência HOST — sessão de execução compartilhada

Data: 10.09.2026. Ambiente: Ubuntu 22.04, GCC 11.4.0, CMake, FMILibrary 3.0.4 na revisão imutável configurada pelo projeto. Escopo: software HOST.

## Procedimento executado

Foi configurado um diretório temporário com runner, FMILibrary obtida pela opção do projeto e testes habilitados. A compilação do runner e da biblioteca `microhil_execution_session` terminou com sucesso. Em seguida, foi executado o CTest completo e uma abertura/encerramento do menu terminal.

O novo teste `execution_session_lifecycle` carregou a fixture FMI 2.0 Co-Simulation, listou suas saídas numéricas, configurou passo de 10 ms e duração de 20 ms, preparou a FMU, iniciou a sessão e confirmou duas etapas concluídas em estado `Finished`.

## Resultado observado

O build foi concluído e **47/47 testes CTest passaram**, incluindo `execution_session_lifecycle`. O executável terminal iniciou e encerrou normalmente pelo menu. A GUI compilou e permaneceu ativa por três segundos em `QT_QPA_PLATFORM=offscreen`. A CLI agora delega preparação, logging, início, Stop e agregação de resultado a `execution_session`; a base Qt já usa essa sessão para carregar uma FMU.

O teste documental `node test/run_spec_tests.js` passou em **29/29** critérios. A execução adicional do motor `onp-spec` não iniciou sua verificação: a instalação local falhou ao importar `run` de `lib/src/cli.js`, por incompatibilidade ESM/CommonJS no Node 18.20.8. Portanto, não há alegação de `onp-spec audit --ci` aprovado nesta revisão.

## Limites

Esta evidência não executa a GUI visualmente, a DAQC, CH340, ESP32, Agent micro-ROS, ROS 2 em enlace, nem mede deadline, jitter ou comportamento elétrico. Os avisos exibidos por headers de terceiros da FMILibrary não são falhas da compilação do código próprio.
