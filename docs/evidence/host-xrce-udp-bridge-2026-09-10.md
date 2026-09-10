# Ponte XRCE UDP no HOST — 10.09.2026

- Ambiente: Ubuntu 22.04; GCC 11.4; CMake; loopback IPv4 local.
- Escopo: ponte bidirecional entre callback MID 04 do coordenador e datagramas UDP em `127.0.0.1`, sem abrir TTY adicional.

## Procedimento executado

1. Criar um socket UDP local que simula o endpoint do Agent.
2. Iniciar a ponte com porta local configurada.
3. Entregar payload XRCE da DAQC ao callback e conferir o datagrama recebido pelo endpoint simulado.
4. Enviar datagrama de retorno do endpoint para a porta efêmera da ponte e verificar o reenquadramento como MID 04 no mailbox do coordenador.
5. Compilar o runner e executar CTest e o harness documental.

## Resultado observado

O payload DAQC→Agent e a resposta Agent→DAQC percorreram a ponte sem alteração. A resposta foi serializada pelo coordenador como quadro XRCE MID 04. A suíte CTest concluiu **46/46** testes e o harness documental concluiu **29/29** verificações.

## Limites

O endpoint foi um socket de teste, não o Micro-ROS Agent. Não houve execução do Agent, ROS 2, CH340, ESP32, DAQC, tópicos, configuração `DaqcSetup`, bancada, HIL ou medição temporal. O teste só demonstra a ponte local e não a interoperabilidade XRCE completa.
