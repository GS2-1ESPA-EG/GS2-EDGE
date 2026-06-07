# Wokwi — Firmware do ESP32

Esta pasta contém os arquivos necessários para reproduzir o circuito do compartimento M-03 da Cápsula Dragon no simulador Wokwi.

## Como usar

1. Acesse https://wokwi.com.
2. Crie um projeto novo selecionando **ESP32**.
3. Cole o conteúdo de `sketch.ino` na aba do editor de código.
4. Cole o conteúdo de `diagram.json` na aba **diagram.json**.
5. Abra o **Library Manager** e adicione (ou edite o `libraries.txt`):
   - **DHT sensor library** (Adafruit)
   - **Adafruit MPU6050**
   - **PubSubClient** (Nick O'Leary)
6. Clique em **▶ Play** para iniciar a simulação.

## Antes de rodar

No `sketch.ino`, confirme o **endereço do broker MQTT**. Existem dois cenários:

### Cenário 1 — FIWARE rodando na Azure (padrão do projeto)

```cpp
const char* MQTT_BROKER = "158.23.61.7";   // IP público da VM
const int   MQTT_PORT   = 1883;             // porta padrão Mosquitto
```

### Cenário 2 — FIWARE rodando no seu PC com ngrok

Se você está usando a alternativa local (Docker Desktop + ngrok no Windows), o ESP32 do Wokwi não acessa o seu `localhost` diretamente — precisa passar pela URL pública do ngrok. Confira a saída do comando `ngrok tcp 1883`:

```
Forwarding   tcp://0.tcp.sa.ngrok.io:18234 -> localhost:1883
```

E ajuste no sketch:

```cpp
const char* MQTT_BROKER = "0.tcp.sa.ngrok.io";   // host gerado pelo ngrok
const int   MQTT_PORT   = 18234;                  // porta gerada pelo ngrok
```

> ⚠️ A porta gerada pelo ngrok muda toda vez que você reinicia o túnel. Sempre confirme antes de rodar o Wokwi.

### Sempre verificar

O **API key** e **device_id** devem bater com o que foi provisionado no Postman:

```cpp
const char* API_KEY   = "4jggokgpepnvsb2uv4s40d59ov";
const char* DEVICE_ID = "compartment_m03";
```

## Pinagem

| Sensor / Atuador | Pino do ESP32 | Observação                  |
|------------------|---------------|-----------------------------|
| DHT22            | GPIO 15       | Temperatura + umidade       |
| MPU6050 SDA      | GPIO 21       | I2C                         |
| MPU6050 SCL      | GPIO 22       | I2C                         |
| LDR (AO)         | GPIO 34       | ADC (0–4095)                |
| LED              | GPIO 2        | Com resistor 220Ω em série  |
| Buzzer           | GPIO 4        | Acionado por `tone()`       |

## Verificação no Serial Monitor

Após dar **Play**, espere ~5 segundos. O Serial Monitor deve mostrar:

```
[WiFi] OK - IP: 10.13.x.x
[MQTT] Conectando ao broker 158.23.61.7:1883 ... conectado!
[MQTT] Assinado em /4jggokgpepnvsb2uv4s40d59ov/compartment_m03/cmd
#1 [PUB OK]   T=5.0 H=50.0 g=1.00 L=512 O=false sev=nominal
```

Se aparecer `[PUB OK]`, o ESP32 está publicando para a Azure.

## Como simular um alerta

1. Passe o mouse sobre o sensor **DHT22** no diagrama.
2. Digite uma temperatura fora da faixa (ex: **12.5**) no campo Temperature.
3. Em ~5 segundos, o LED acende vermelho, o buzzer toca e a `severity` muda para `critical`.
4. No Postman, executando o GET da entidade, o `current_temp` aparecerá `12.5` e `severity` `"critical"`.
