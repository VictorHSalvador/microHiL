# Correção RX do harness CONFIG — 13.09.2026

## Defeito e correção

`read_disable_confirmation()` lia bytes da UART e os registrava na captura, mas não os adicionava ao `buffer` entregue a `extract_frames()`. Assim, uma confirmação CONFIG recebida pela porta não chegava ao parser. A rotina agora executa `buffer.extend(chunk)` antes de extrair frames.

## Verificação HOST

O teste do harness cobre confirmação CONFIG `59 72 01 01 01` recebida como um único chunk e em dois fragmentos. Em ambos os casos a confirmação é reconhecida. A verificação não usa CH340, UART, ESP32, Agent, micro-ROS ou ROS.

## Limites

O procedimento manual já havia observado resposta da UART. Esta correção elimina uma causa de falso negativo no diagnóstico HOST, mas não é novo ensaio físico e não valida STREAMING, DATA, I/O, timing ou HIL.
