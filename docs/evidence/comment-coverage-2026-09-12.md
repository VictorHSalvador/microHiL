# Cobertura de comentários — 12.09.2026

Escopo: arquivos próprios de produção em `src`, `include`, `gui` e `firmware/daqc_esp32/main`.

Procedimento: revisão das alterações locais, restauração autorizada dos cinco arquivos que continham comentários extensos e quebra cosmética, e inclusão de comentário técnico conciso nos arquivos sem comentário. A constituição e ADR-001 confirmam que comentários de desenvolvedor podem ser escritos em português ou inglês, desde que expliquem contrato, intenção ou módulo e não narrem sintaxe básica.

Resultado: todos os arquivos do escopo contêm ao menos um comentário. O build limpo concluiu e o CTest executou 45 testes com sucesso; um teste XRCE foi pulado pela configuração. O teste documental concluiu 29 verificações. A GUI foi compilada e iniciada por cinco segundos em `QT_QPA_PLATFORM=offscreen`.

Limites: a inspeção não prova cobertura semântica de cada função nem valida GUI visual, FMU, DAQC, ESP32, XRCE, sinais físicos, deadlines ou HIL.
