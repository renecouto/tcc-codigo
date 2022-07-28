# tcc
Todas as credenciais foram removidas dos códigos.

## influxdb-benchmark
utilizar o comando dentro da pasta influxdb-benchmark `docker-compose up` para subir influxdb e mqtt localmente. instalar docker-compose: https://docs.docker.com/compose/install/

O script influxdb-benchmark/benchmark.py insere 1000 pontos por segundo no influxdb. Estando na pasta influxdb-benchmark, para instalar as dependências execute `pip install -r requirements.txt`, para rodar o script, `python3 benchmark.py`

## spi-influxdb
Programa para esp32 master lendo SPI e enviando para influxdb. Utilizar PlatformIO para rodar e testar. Instalar platformIO: https://platformio.org/install

## spi-mqtt
Programa para esp32 master lendo SPI e enviando para MQTT. Utilizar PlatformIO para rodar e testar


## spi-mqtt-v2
Programa para esp32 master lendo SPI e enviando para MQTT a uma taxa de 2000 pontos por segundo, com tentativas de otmizações. Utilizar PlatformIO para rodar e testar

## spislave8266pio
Programa para esp32 slave retornando dados através de spi. Utilizar PlatformIO para rodar e testar.

## mqtt-example
Necessita instalar cargo e rust https://www.rust-lang.org/tools/install
gera binários pub e sub. para rodar sub, que recebe mensagens de dados e insere no influxdb, rodar `cargo run --bin sub`.

para rodar pub, que publica comandos para a ESP32 atraves de mqtt, rodar `cargo run --bin pub <comando>`.