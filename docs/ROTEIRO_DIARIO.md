# Roteiro rápido — ligar e desligar o sistema

Este é o checklist enxuto para uso diário, depois que tudo já foi instalado uma vez.

Tem **dois cenários**, escolha o que se aplica a você:

- [Cenário A — FIWARE na Azure (recomendado pelo projeto)](#cenário-a--fiware-na-azure)
- [Cenário B — FIWARE rodando local no seu PC Windows](#cenário-b--fiware-local-no-windows)

---

## Cenário A — FIWARE na Azure

### Ligar (toda vez que for trabalhar)

#### 1. Portal Azure
- Acesse `portal.azure.com`
- **Virtual machines → myVm**
- Se status = **Stopped/Deallocated** → clique em **Start** e espere 1-2 min
- Anote o **Public IP** (se mudou, atualize o `sketch.ino` do Wokwi e o environment do Postman)

#### 2. SSH na VM (PowerShell)
```powershell
ssh -i "C:\Users\Administrador\Downloads\dev\orbitstock\keys\GS2VM_KEY.pem" GS2VM@158.23.61.7
```

#### 3. Verificar containers FIWARE
```bash
cd fiware
sudo docker ps
```

Se algum dos 6 containers (orion, mongo-internal, mongo-historical, iot-agent, sth-comet, mosquitto) estiver parado:
```bash
sudo docker compose up -d
sudo docker ps    # confere
```

#### 4. Confirmar provisionamento no Postman
- **2.2 Listar Service Groups** → deve mostrar 1 service com `resource: /iot/d`
- **2.4 Listar Devices** → deve mostrar `compartment_m03` com `entity_name: urn:ngsi-ld:Compartment:DRAGON-C209:M-03` e 6 attributes preenchidos

Se sumiu algo, reexecute **2.1, 2.3** (não precisa do 2.5 se a entidade ainda está no Orion).

#### 5. Ligar o Wokwi
- Abra o projeto no Wokwi
- Clique em **▶ Play**
- No Serial Monitor confirme: `[WiFi] OK`, `[MQTT] conectado`, `#1 [PUB OK]`

#### 6. Validar fluxo
- Postman: **3.1 Estado atual do compartimento M-03**
- Os campos `current_temp`, `current_humidity`, etc. devem refletir os valores do ESP32 e mudar a cada 5 segundos

✅ Tudo pronto pra trabalhar.

### Desligar (toda vez que terminar)

#### 1. Parar o Wokwi
- Botão ⏹ Stop no simulador

#### 2. Parar a VM (importante pra economizar crédito)
- `portal.azure.com → myVm → Stop` (botão em cima do painel)
- Confirme quando aparecer o popup

> VM ligada 24/7 = ~US$ 30/mês
> VM parada = centavos por dia (só o disco)

✅ Crédito Azure preservado.

---

## Cenário B — FIWARE local no Windows

### Ligar (toda vez que for trabalhar)

#### 1. Subir o Docker Desktop
- Abra o aplicativo Docker Desktop
- Aguarde aparecer "Docker is running" na bandeja do sistema

#### 2. Subir os containers do FIWARE
No PowerShell:
```powershell
cd C:\dev\orbitstock-edge\fiware
docker compose up -d
docker ps    # confere os 6 containers
```

#### 3. Subir o túnel ngrok pro Mosquitto
**Em uma janela separada do PowerShell** (deixe aberta o dia todo):
```powershell
cd C:\ngrok
.\ngrok.exe tcp 1883
```

Anote o host e porta que aparecem:
```
Forwarding   tcp://0.tcp.sa.ngrok.io:18234 -> localhost:1883
                  ^^^^^^^^^^^^^^^^^^   ^^^^^
                  HOST PRO ESP32       PORTA PRO ESP32
```

> ⚠️ A porta gera nova a cada vez que você roda `ngrok tcp 1883`. **Anota a porta nova.**

#### 4. Atualizar o sketch do ESP32 no Wokwi
Edita as linhas:
```cpp
const char* MQTT_BROKER = "0.tcp.sa.ngrok.io";   // host do ngrok
const int   MQTT_PORT   = 18234;                  // porta NOVA do ngrok
```

#### 5. Confirmar provisionamento no Postman
- Garante que o environment está com `url = localhost`
- **2.2 Listar Service Groups** → 1 service com `resource: /iot/d`
- **2.4 Listar Devices** → device `compartment_m03` com `entity_name` correto

Se sumiu, reexecute **2.1, 2.3**.

#### 6. Ligar o Wokwi
- Clica em **▶ Play**
- Serial Monitor: `[WiFi] OK`, `[MQTT] conectado`, `[PUB OK]`

#### 7. Validar fluxo
- Postman: **3.1 Estado atual** → valores chegando

✅ Tudo pronto.

### Desligar

```powershell
# Para o ngrok: Ctrl+C na janela dele

# Para o FIWARE:
cd C:\dev\orbitstock-edge\fiware
docker compose down

# Para o Wokwi: ⏹ Stop

# (Opcional) Sair do Docker Desktop pela bandeja
```

### Reset total (apaga tudo!)
```powershell
cd C:\dev\orbitstock-edge\fiware
docker compose down -v   # -v apaga os volumes (perde todos os dados)
```

Depois disso você precisa refazer o provisionamento (Service Group + Device + Entidade) no Postman.

---

## Em caso de erro (qualquer cenário)

Veja o **README.md** principal, seção **Troubleshooting**.

Resumo dos comandos mais úteis pra investigar:

### Logs dos containers
```bash
# Azure (dentro da VM via SSH):
sudo docker logs fiware-iot-agent --tail 30 --since 1m

# Windows (no PowerShell):
docker logs fiware-iot-agent --tail 30 --since 1m
```

### Reiniciar um container específico
```bash
sudo docker restart fiware-iot-agent    # Azure
docker restart fiware-iot-agent          # Windows
```

### Subir tudo do zero (NÃO PERDE DADOS - volumes preservados)
```bash
sudo docker compose down && sudo docker compose up -d    # Azure
docker compose down && docker compose up -d              # Windows
```

### Limpeza total (PERDE TUDO!)
```bash
sudo docker compose down -v    # Azure
docker compose down -v          # Windows
```
