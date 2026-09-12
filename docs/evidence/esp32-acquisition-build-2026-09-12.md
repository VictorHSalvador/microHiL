# Evidência de build — firmware ESP32 com aquisição configurável

Data: 12.09.2026. Ambiente: Ubuntu 22.04, ESP-IDF v5.2.6 na tag oficial `9ef24e3e2a2c96e720d83c574a3f8699177573da`, ferramentas temporárias em `/tmp/esp-idf-tools` e fonte do SDK em `/tmp/esp-idf-v5.2.6`.

## Procedimento e resultado observado

1. O SDK e sua toolchain foram recriados após a reinicialização que removeu o ambiente temporário anterior.
2. Os artefatos gerados do componente `micro_ros_espidf_component` foram limpos e recriados a partir de `extra_ros_packages/microhil_interfaces`, para que a definição gerada de `DaqcSetup` incluísse `acquisition_frequency_hz`.
3. A primeira compilação identificou corretamente a incompatibilidade entre o fonte atual e a interface gerada antiga: o membro `acquisition_frequency_hz` não existia no tipo gerado.
4. Após a regeneração, `idf.py build` concluiu sem erro para `firmware/daqc_esp32`. A imagem `build/microhil_daqc_esp32.bin` ocupou `0x3bd20` bytes da partição de aplicação de `0x100000`, com `0xc42e0` bytes livres.

## Limites

O resultado confirma a compilação cruzada do firmware atual e a coerência do campo de frequência com a interface micro-ROS regenerada. Não houve gravação desta imagem, abertura da CH340, Agent, sessão XRCE, publicação de tópicos, aplicação de perfil, DATA, READ_ACK, medição de aquisição, I/O, bancada ou HIL. Avisos provenientes de dependências de terceiros micro-ROS não foram promovidos a evidência de comportamento do produto.
