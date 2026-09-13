# Ensaio CONFIG com tentativas da DAQC — 13.09.2026

## Procedimento e resultado

A imagem 0.26.7 foi compilada e gravada na ESP32 pela CH340, com hashes confirmados. Após reset, espera de 1 s e cinco envios de CONFIG DISABLE `59 72 01 01` em intervalo de 200 ms, as cinco respostas foram `59 72 01 01 01`. Sem reset, dez tentativas receberam nove confirmações. Houve tráfego MID 04 de inicialização na mesma janela.

O resultado identifica a limitação do harness anterior: ele enviava apenas um CONFIG depois de 200 ms. A imagem 0.26.7 permanece gravada.

## Limites

O ensaio confirma somente a resposta CONFIG nesse procedimento. Não confirma sessão XRCE, Agent, tópicos ROS, perfil aplicado, DATA, I/O, timing ou HIL.
