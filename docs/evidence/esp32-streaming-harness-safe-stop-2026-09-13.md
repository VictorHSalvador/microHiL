# Correção de encerramento seguro do harness STREAMING — 13.09.2026

## Alteração

O harness `tools/esp32_streaming_smoke.py` passou a solicitar CONFIG DISABLE em um bloco `finally` depois que STREAMING é aceito. Assim, uma falha de leitura serial, ponte UDP ou parser durante a coleta não ignora a tentativa de parada segura. A limpeza fecha a ponte UDP e a porta mesmo quando a confirmação DISABLE não é recebida.

## Verificação HOST

Foram executados `python3 -m unittest discover -s tests` e `node test/run_spec_tests.js`. Os testes do harness cobrem a confirmação DISABLE e a rejeição de uma resposta em outro estado.

## Limitações

Esta alteração foi verificada no HOST. Ela não repete o ensaio físico STREAMING de 0.26.11 nem mede o nível elétrico dos pinos após uma falha de comunicação.
