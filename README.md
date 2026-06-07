# OrbitStock — Edge Computing

Sistema inteligente de gestão de carga para a Cápsula Dragon (SpaceX), entregue para a disciplina **Edge Computing & Computer Systems** da FIAP — parte da Global Solution 2026.

> **Autores**
> - Enrico Dellatorre — RM566824
> - Gustavo Hiruo — RM567625
>
> Turma: 1ESPA — Engenharia de Software
> Data: Maio/2026

---

## Sumário

1. [Visão geral](#visão-geral)
2. [Arquitetura](#arquitetura)
3. [Conteúdo do repositório](#conteúdo-do-repositório)
4. [Pré-requisitos](#pré-requisitos)
5. [Setup completo (do zero)](#setup-completo-do-zero)
6. [Como usar no dia a dia](#como-usar-no-dia-a-dia)
7. [Testando os fluxos](#testando-os-fluxos)
8. [Troubleshooting](#troubleshooting)
9. [Próximos passos (Global Solution)](#próximos-passos-global-solution)

---

## Visão geral

O **OrbitStock** monitora compartimentos de carga a bordo da Cápsula Dragon. Cada compartimento tem um ESP32 conectado a sensores ambientais que publica telemetria via MQTT para uma instância do FIWARE rodando na nuvem.

Esta entrega cobre o **compartimento M-03 (médico refrigerado)** da Dragon C209, com sensoriamento de temperatura, umidade, impacto, luminosidade e abertura.

O sistema demonstra os dois sentidos do fluxo edge-cloud:

- **Edge → Cloud:** ESP32 publica leituras MQTT que são automaticamente convertidas para o modelo NGSI e atualizam a entidade no Orion Context Broker.
- **Cloud → Edge:** comandos enviados pelo Postman ao Orion são propagados de volta ao ESP32 para acionar atuadores (LED + buzzer).

---

## Arquitetura

```
┌────────────────────────┐
│  Bordo — Capsula Dragon│
│                        │
│  ESP32 + sensores      │  ──► Triggers locais (LED, buzzer)
│  (DHT22, MPU6050, LDR) │       quando severity == "critical"
└──────────┬─────────────┘
           │ MQTT (Ultralight 2.0)
           │ /<apikey>/<device>/attrs
           ▼
┌────────────────────────────────────────────────┐
│   Cloud — VM Azure (FIWARE Descomplicado)      │
│                                                │
│   Mosquitto :1883  ─► IoT Agent UL :4041       │
│                          │                     │
│                          ▼ NGSI                │
│                       Orion :1026 ─► MongoDB   │
│                          │                     │
│                          └─► STH-Comet :8666   │
│                              (historico)       │
└────────────────────────────────────────────────┘
           ▲                       │
           │ comandos              │ HTTP queries
           │ (Postman)             ▼
           └─────────────────► Dashboard / Backend
                               (proxima fase)
```

### Stack

| Camada              | Tecnologia                                    |
|---------------------|-----------------------------------------------|
| Hardware            | ESP32 DevKit V1 (simulado no Wokwi)           |
| Sensores            | DHT22, MPU6050, LDR + LED, buzzer             |
| Protocolo de campo  | MQTT 3.1.1 sobre TCP                          |
| Payload             | Ultralight 2.0 (`t|5.1|h|47.5|...`)           |
| Broker MQTT         | Eclipse Mosquitto                             |
| Tradutor            | FIWARE IoT Agent UL                           |
| Context Broker      | FIWARE Orion (NGSI-v2)                        |
| Histórico           | FIWARE STH-Comet                              |
| Persistência        | MongoDB 4.4                                   |
| Hospedagem          | Microsoft Azure (VM Ubuntu 24.04 LTS, B2s)    |
| Cliente de testes   | Postman                                       |

### Entidade NGSI

```
id    : urn:ngsi-ld:Compartment:DRAGON-C209:M-03
type  : Compartment

# Atributos estáticos (definidos no cadastro)
spacecraft_id    = "DRAGON-C209"
compartment_id   = "M-03"
name             = "Medico refrigerado"
category         = "medical"
temp_min         = 2.0
temp_max         = 8.0
humidity_max     = 80.0

# Atributos dinâmicos (atualizados pelo ESP32 a cada 5s)
current_temp     ← t
current_humidity ← h
last_impact_g    ← g
last_light_level ← l
is_open          ← o
severity         ← s

# Comandos (recebidos do Orion via MQTT)
alert  → liga LED + buzzer no ESP32
ack    → desliga atuadores (alerta reconhecido)
```

---

## Conteúdo do repositório

```
orbitstock-edge/
├── README.md                       ← este arquivo
├── wokwi/
│   ├── sketch.ino                  ← firmware completo do ESP32
│   ├── diagram.json                ← layout do circuito para o Wokwi
│   ├── libraries.txt               ← libs necessárias (DHT, MPU, PubSubClient)
│   └── README.md
├── postman/
│   ├── OrbitStock.postman_collection.json    ← coleção com todas as requests
│   ├── OrbitStock-FIWARE.postman_environment.json  ← env com {{url}}
│   └── README.md
├── fiware/
│   ├── docker-compose.yml          ← stack Docker do FIWARE (Orion, IoT Agent, STH, Mosquitto)
│   ├── mosquitto/
│   │   └── mosquitto.conf
│   ├── LICENSE                      ← MIT (Fabio Cabrini)
│   └── README.md                    ← crédito + instruções
└── docs/
    └── ROTEIRO_DIARIO.md           ← checklist diário
```

A pasta `fiware/` é uma **cópia adaptada do FIWARE Descomplicado** do Prof. Fábio Cabrini (licença MIT preservada). Isso torna o projeto autocontido — quem clona o repositório consegue subir o ambiente FIWARE sem precisar baixar separadamente de outro lugar.

---

## Pré-requisitos

- Conta **Microsoft Azure for Students** (créditos grátis com e-mail FIAP)
- Conta no **Wokwi** (gratuita)
- **Postman** instalado (desktop ou web)
- **Cliente SSH** (o PowerShell do Windows 10/11 já tem por padrão)
- Repositório do **FIWARE Descomplicado**: https://github.com/fabiocabrini/fiware

---

## Setup completo (do zero)

### 1. Provisionar a VM na Azure

1. Acesse `portal.azure.com` logado com o e-mail FIAP.
2. **Create a resource → Ubuntu Server 22.04 LTS**.
3. Configuração mínima:
   - **Resource group:** `rg-orbitstock`
   - **VM name:** `myVm`
   - **Region:** Brazil South
   - **Size:** Standard B2s (2 vCPU, 4 GB RAM)
   - **Authentication:** SSH public key (a Azure gera e oferece para download — **baixe o `.pem` e guarde**)
   - **Inbound ports:** apenas `SSH (22)`
4. Após o deploy, anote o **Public IP address** (no nosso caso: `158.23.61.7`).

### 2. Abrir portas adicionais no NSG

No painel da VM, vá em **Networking → Network settings → Add inbound port rule** e abra:

| Porta | Serviço          |
|-------|------------------|
| 1026  | Orion            |
| 1883  | Mosquitto (MQTT) |
| 4041  | IoT Agent        |
| 8666  | STH-Comet        |

### 3. Configurar permissões da chave SSH (Windows)

A primeira conexão SSH falha se a `.pem` estiver com permissões abertas. No PowerShell:

```powershell
icacls "C:\caminho\para\GS2VM_KEY.pem" /inheritance:r
icacls "C:\caminho\para\GS2VM_KEY.pem" /grant:r "${env:USERNAME}:(R)"
```

### 4. Conectar via SSH

```powershell
ssh -i "C:\caminho\para\GS2VM_KEY.pem" GS2VM@158.23.61.7
```

> Substitua `GS2VM` pelo usuário que aparece em **Connect → Native SSH** no portal Azure.

### 5. Instalar o FIWARE dentro da VM

```bash
# Atualizar pacotes (pode levar 3-5 min)
sudo apt update && sudo apt upgrade -y

# Instalar Docker, Compose e Git
sudo apt install -y docker.io docker-compose-v2 git

# Habilitar Docker no boot
sudo systemctl enable docker
sudo systemctl start docker

# Reiniciar se foi pedido System restart required
sudo reboot
```

Após reconectar, você tem **duas opções equivalentes** para obter a stack FIWARE:

**Opção A — Usar a pasta `fiware/` deste repositório (autocontido):**

```bash
# Clone este repositorio
git clone https://github.com/SEU_USUARIO/orbitstock-edge.git
cd orbitstock-edge/fiware

# Sobe os 6 containers (Orion, IoT Agent UL, STH-Comet, Mosquitto, 2x MongoDB)
sudo docker compose up -d

# Confirma que todos estão Up (healthy)
sudo docker ps
```

**Opção B — Clonar diretamente do Prof. Fábio Cabrini (recomendado pela disciplina):**

```bash
git clone https://github.com/fabiocabrini/fiware.git
cd fiware
sudo docker compose up -d
sudo docker ps
```

As duas opções sobem **exatamente os mesmos containers** (a pasta `fiware/` deste repositório é uma cópia do FIWARE Descomplicado, com a licença MIT preservada). A opção A é útil se você quer um repositório totalmente autocontido; a opção B é a forma original que a disciplina pede.

### 6. Health Check

Dentro da VM:

```bash
curl http://localhost:1026/version   # Orion responde com JSON
```

Do seu navegador (qualquer máquina):

```
http://158.23.61.7:1026/version
http://158.23.61.7:8666/version
```

Ambos devem retornar a versão.

### 7. Importar a coleção Postman

1. Abra o Postman.
2. **Collections → Import** → arraste `postman/OrbitStock.postman_collection.json`.
3. **Environments → Import** → arraste `postman/OrbitStock-FIWARE.postman_environment.json`.
4. No canto superior direito, selecione o environment **OrbitStock-FIWARE**.
5. Se o IP da sua VM for diferente de `158.23.61.7`, edite a variável `url` no environment.

### 8. Provisionar entidade, service group e device

Na coleção Postman, execute em ordem:

1. **2.1 Criar Service Group (UL /iot/d)** → espera `201`
2. **2.3 Criar Device compartment_m03** → espera `201`
3. **2.5 Criar Entidade no Orion** → espera `201`

Confirmação (todos `200 OK`):

- **2.2 Listar Service Groups** — deve mostrar `resource: /iot/d`, apikey configurada
- **2.4 Listar Devices** — deve mostrar `entity_name: urn:ngsi-ld:Compartment:DRAGON-C209:M-03`, 6 attributes e 2 commands
- **3.1 Estado atual do compartimento M-03** — deve mostrar a entidade

### 9. Subir o circuito no Wokwi

1. Acesse https://wokwi.com e crie um novo **ESP32** project.
2. Copie o conteúdo de `wokwi/sketch.ino` para o editor.
3. Copie o conteúdo de `wokwi/diagram.json` para a aba **diagram.json**.
4. Em **Library Manager**, adicione:
   - DHT sensor library
   - Adafruit MPU6050
   - PubSubClient (Nick O'Leary)
5. Clique em **▶ Play**.

No Serial Monitor deve aparecer:

```
============================================
  OrbitStock - Edge Computing
  Spacecraft  : DRAGON-C209
  Compartment : M-03 (Medico refrigerado)
  Broker      : 158.23.61.7:1883
  Publicando  : /4jggokgpepnvsb2uv4s40d59ov/compartment_m03/attrs
  Assinando   : /4jggokgpepnvsb2uv4s40d59ov/compartment_m03/cmd
============================================
[WiFi] OK - IP: 10.13.x.x
[MQTT] Conectando ao broker 158.23.61.7:1883 ... conectado!
[MQTT] Assinado em /4jggokgpepnvsb2uv4s40d59ov/compartment_m03/cmd
#1 [PUB OK]   T=5.0 H=50.0 g=1.00 L=512 O=false sev=nominal
#2 [PUB OK]   T=5.0 H=50.0 g=1.00 L=512 O=false sev=nominal
```

### 10. Validar Edge → Cloud no Postman

Execute **3.1 Estado atual do compartimento M-03**. Os campos `current_temp`, `current_humidity`, `last_impact_g`, `last_light_level`, `is_open`, `severity` devem refletir os valores publicados pelo ESP32 e atualizar a cada 5 segundos.

---

## Alternativa: rodar localmente no Windows (sem Azure)

Esta seção mostra como rodar o **FIWARE inteiro no seu PC Windows** usando Docker Desktop, sem precisar criar VM nem gastar crédito de nuvem. Útil para desenvolvimento, testes ou apresentação offline.

### Diferença crítica em relação à versão Azure

O ESP32 do Wokwi **roda nos servidores do Wokwi** (na nuvem), então ele **não consegue acessar `localhost` ou `192.168.x.x`** do seu PC — está em outra rede. Precisamos expor o FIWARE local com uma **URL pública temporária** usando uma ferramenta chamada **ngrok**.

```
ESP32 (Wokwi, nuvem) ──MQTT──> ngrok ──> FIWARE (seu PC) ←──> Postman (seu PC)
```

### Pré-requisitos

| Software | Onde baixar |
|----------|-------------|
| **Docker Desktop** | https://www.docker.com/products/docker-desktop/ |
| **ngrok** (gratuito) | https://ngrok.com/download |
| **Git** | https://git-scm.com/download/win |
| **Postman** | https://www.postman.com/downloads/ |
| **Conta Wokwi** | https://wokwi.com (gratuita) |
| **Conta ngrok** | https://dashboard.ngrok.com/signup (gratuita, pra pegar authtoken) |

### Passo 1 — Instalar Docker Desktop

1. Baixa o instalador, executa.
2. Marca "Use WSL 2 instead of Hyper-V" durante a instalação.
3. Reinicia o PC quando pedido.
4. Abre o Docker Desktop e aguarda aparecer "Docker is running" (ícone verde na bandeja).

Confirma no PowerShell:
```powershell
docker --version
docker compose version
```

### Passo 2 — Clonar este repositório

```powershell
cd C:\dev
git clone https://github.com/SEU_USUARIO/orbitstock-edge.git
cd orbitstock-edge
```

### Passo 3 — Subir o FIWARE local

```powershell
cd fiware
docker compose up -d
docker ps
```

Esperar até os 6 containers aparecerem como `Up` (especialmente `fiware-orion`, `fiware-iot-agent` e `fiware-mosquitto`).

Health Check pra validar:
```powershell
curl http://localhost:1026/version
curl http://localhost:8666/version
curl http://localhost:4041/iot/about
```

Os três devem retornar JSON. Pronto — **FIWARE rodando no seu PC**.

### Passo 4 — Instalar e configurar ngrok

1. Baixa `ngrok.exe`, descompacta numa pasta (ex: `C:\ngrok\`).
2. Cria conta gratuita em https://dashboard.ngrok.com.
3. Copia seu **authtoken** do dashboard.
4. No PowerShell:

```powershell
cd C:\ngrok
.\ngrok.exe config add-authtoken SEU_TOKEN_AQUI
```

### Passo 5 — Expor o Mosquitto MQTT via ngrok

O Mosquitto roda na porta `1883` (TCP, não HTTP). Pra expor pela ngrok:

```powershell
.\ngrok.exe tcp 1883
```

Vai abrir uma tela mostrando algo tipo:

```
Forwarding   tcp://0.tcp.sa.ngrok.io:18234 -> localhost:1883
```

**Anota esses dois valores:**
- **Host:** `0.tcp.sa.ngrok.io` (varia conforme a região)
- **Porta:** `18234` (sempre diferente a cada execução)

> ⚠️ **Deixa essa janela do PowerShell aberta enquanto estiver usando o sistema.** Se fechar, o túnel cai.

### Passo 6 — Atualizar o sketch do ESP32

No Wokwi, edita o `sketch.ino` e troca as linhas:

```cpp
const char* MQTT_BROKER = "0.tcp.sa.ngrok.io";   // host que o ngrok deu
const int   MQTT_PORT   = 18234;                  // porta que o ngrok deu
```

Salva e dá **Stop** / **Play** no Wokwi.

### Passo 7 — Configurar o Postman pra apontar pro localhost

Edita a variável `url` do environment **OrbitStock-FIWARE** no Postman:

| Versão Azure | Versão Local |
|---|---|
| `158.23.61.7` | `localhost` |

Pronto — todas as requisições da coleção vão pro seu PC.

### Passo 8 — Provisionar e testar

Execute as requisições do Postman exatamente como na versão Azure (passo 8 acima):

1. **2.1 Criar Service Group** → `201`
2. **2.3 Criar Device** → `201`
3. **2.5 Criar Entidade no Orion** → `201`
4. **3.1 Estado atual do compartimento** → vê os valores chegando do ESP32

### Diferenças e cuidados na versão local

| Aspecto | Azure | Local com ngrok |
|---|---|---|
| Custo | Crédito Azure ($30/mês ligado 24/7) | Grátis |
| Disponibilidade externa | URL pública fixa | URL muda cada `ngrok tcp` |
| Latência | Depende da região | Pode ser maior (passa por servidor ngrok) |
| Persistência | Containers + volumes na VM | Containers + volumes no Docker Desktop |
| Quando o PC desliga | VM continua rodando | Tudo para |
| Limite gratuito ngrok | — | 1 túnel TCP simultâneo, sem domínio fixo |

### Parar tudo

```powershell
# Parar o ngrok: Ctrl+C na janela
# Parar o FIWARE:
cd C:\dev\orbitstock-edge\fiware
docker compose down

# Limpeza total (apaga dados!):
docker compose down -v
```

### Reabrir o sistema dia seguinte

```powershell
# 1. Sobe o FIWARE
cd C:\dev\orbitstock-edge\fiware
docker compose up -d

# 2. Liga o ngrok (vai dar uma NOVA porta TCP!)
cd C:\ngrok
.\ngrok.exe tcp 1883

# 3. Atualiza o MQTT_PORT no sketch.ino do Wokwi com a porta nova
# 4. Play no Wokwi
```

> 💡 **Dica:** o plano gratuito do ngrok gera uma URL/porta diferente toda vez que você inicia. Se quiser uma URL **fixa**, o plano pago (~$8/mês) oferece "reserved domains".

---

## Como usar no dia a dia

### Roteiro de "início do dia"

1. **portal.azure.com → VM `myVm` → Start** (se estiver Stopped).
2. **SSH** na VM:
   ```powershell
   ssh -i "C:\caminho\para\GS2VM_KEY.pem" GS2VM@158.23.61.7
   ```
3. **Verificar containers:**
   ```bash
   cd fiware
   sudo docker ps
   ```
   Se algum estiver parado: `sudo docker compose up -d`.
4. **Postman:** rode **2.2 Listar Service Groups** e **2.4 Listar Devices** para confirmar que o provisionamento sobreviveu (geralmente sobrevive — o MongoDB persiste em volume).
5. **Wokwi:** clique em **▶ Play**.
6. **Postman:** **3.1 Estado atual do compartimento M-03** — confirme que os valores mudam.

### Roteiro de "fim do dia" (economizar crédito Azure)

1. **Wokwi:** clique em **⏹ Stop**.
2. **portal.azure.com → VM `myVm` → Stop** *(VM parada gasta centavos por dia em vez de ~US$ 1/dia)*.

---

## Testando os fluxos

### Fluxo Edge → Cloud (telemetria automática)

1. Com Wokwi rodando, no Wokwi passe o mouse sobre o sensor **DHT22**.
2. Digite **12.5** no campo Temperature.
3. Aguarde ~10 segundos.
4. No Postman, execute **3.1 Estado atual do compartimento M-03**.

Resultado esperado:

- `current_temp.value` = `12.5`
- `severity.value` = `"critical"`
- No Wokwi: LED vermelho aceso e buzzer tocando (triggers locais)

### Fluxo Cloud → Edge (comando do Postman ao ESP32)

1. Com Wokwi rodando, execute **4.1 Acionar ALERT**.
2. Observe o Serial Monitor do Wokwi: deve aparecer
   ```
   >> Comando recebido em /4jgg.../compartment_m03/cmd: compartment_m03@alert|on
      -> ALERTA ATIVADO via cloud
   ```
3. O LED acende e o buzzer toca, mesmo com temperatura nominal.
4. Execute **4.2 Acionar ACK** para reconhecer e desligar:
   ```
   >> Comando recebido em /4jgg.../compartment_m03/cmd: compartment_m03@ack|true
      -> ALERTA RECONHECIDO via cloud
   ```

---

## Troubleshooting

### Erro de conexão SSH

```
UNPROTECTED PRIVATE KEY FILE!
```
**Solução:** rode o `icacls` do passo 3 do setup.

```
Connection timed out
```
**Solução:** VM provavelmente está parada (Stopped). Vá no portal Azure e dê Start.

### IoT Agent não atualiza a entidade

Olhe os logs:

```bash
sudo docker logs fiware-iot-agent --tail 30 --since 1m
```

**Erro `MEASURES-004: Device not found`** → o service group ou device não está provisionado para o `resource` `/iot/d`. Reexecute **5.1, 5.2, 5.3** (deletes) e depois **2.1, 2.3, 2.5** na ordem.

**Entidade duplicada (`Compartment:compartment_m03` aparece junto com `urn:ngsi-ld:...`)** → o auto-provisioning do IoT Agent criou uma entidade sem o `entity_name` correto. **Pare o Wokwi**, execute **5.1, 5.4**, depois **2.3** (recriar device) garantindo que o body tem o `entity_name` completo e a `apikey`.

### Postman retorna 404 no GET da entidade

Verifique que os headers `fiware-service: smart` e `fiware-servicepath: /` estão na aba **Headers** (não em **Params**). Esse é o erro mais comum.

### ESP32 não conecta no MQTT (`rc=-2`)

- Porta 1883 não foi aberta no NSG da Azure (volte ao passo 2 do setup).
- O IP no `sketch.ino` está diferente do IP atual da VM. Se a VM foi destruída/recriada, o IP pública pode mudar.

---

## Créditos

### FIWARE Descomplicado

A pasta `fiware/` deste repositório (incluindo `docker-compose.yml` e `mosquitto/mosquitto.conf`) é uma cópia adaptada do projeto **FIWARE Descomplicado**, de autoria do **Prof. Fábio Henrique Cabrini** (FIAP), licenciado sob a **MIT License**:

- Repositório original: https://github.com/fabiocabrini/fiware
- Licença completa preservada em: `fiware/LICENSE`

Os arquivos foram incluídos neste repositório para torná-lo autocontido (qualquer pessoa que clone consegue subir o ambiente sem precisar de outro repositório), com crédito explícito ao autor original. Nenhuma modificação substantiva foi feita — apenas comentários explicativos.