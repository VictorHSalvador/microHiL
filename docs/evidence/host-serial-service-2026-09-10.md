# Evidência HOST — worker serial do coordenador DAQC

Data: 10.09.2026  
Ambiente: Ubuntu 22.04.5 x86-64; GCC 11.4.0; kernel 6.8.0-138-generic; build isolado em `/tmp/microhil-serial-service`.

## Procedimento executado

1. Configurar os componentes HOST sem runner FMU, com `BUILD_TESTING=ON`.
2. Compilar o worker `daq_serial_service` junto aos componentes do coordenador.
3. Executar `ctest --test-dir /tmp/microhil-serial-service --output-on-failure`.
4. Em `daq_serial_service_round_trip`, criar pseudo-terminal Linux, iniciar o único worker proprietário do lado escravo a 152.000 bit/s, enviar confirmações ENABLE e STREAMING pelo lado mestre e publicar uma saída DATA pelo coordenador.
5. Confirmar no lado mestre o frame DATA com SYNC, MID, SEQ e payload esperados; encerrar o worker antes de fechar a TTY.

## Resultado observado

O build concluiu e o CTest executou 40 testes com sucesso. A thread de comunicação recebeu os dois CONFIG, aplicou STREAMING no coordenador, transmitiu o DATA pendente e contabilizou bytes recebidos e quadro enviado. O teste encerrou a thread antes do fechamento do descritor.

## Limites

O pseudo-terminal não comprova CH340, ESP32, taxa efetiva, fragmentação XRCE, Agent, firmware, FMU, prioridade do escalonador, deadlines ou sinais físicos. O worker serial é distinto da thread de simulação, mas ainda não está conectado ao ciclo de execução do runner nem a um dispositivo físico.
