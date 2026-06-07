# FIWARE — Stack de containers Docker

Esta pasta contém a configuração Docker Compose necessária para subir o ambiente FIWARE inteiro (Orion, IoT Agent UL, STH-Comet, Mosquitto, MongoDB) que o OrbitStock usa como backend de IoT.

## Crédito ao autor original

Este `docker-compose.yml` e o `mosquitto.conf` são adaptados do projeto **FIWARE Descomplicado**, de autoria do **Prof. Fábio Henrique Cabrini** (FIAP), licenciado sob MIT:

> https://github.com/fabiocabrini/fiware

Foi reproduzido aqui (com a licença MIT preservada em `LICENSE`) para tornar o OrbitStock autocontido — quem clonar este repositório consegue subir o ambiente sem precisar baixar separadamente do repositório original.

Se você estiver no contexto da disciplina **Edge Computing & Computer Systems** da FIAP, **o caminho oficial recomendado pelo professor é clonar diretamente do repositório dele**:

```bash
git clone https://github.com/fabiocabrini/fiware.git
```

Esta cópia local serve principalmente para:

1. Reprodutibilidade fora do contexto da aula
2. Garantir que o projeto continue funcionando mesmo que o repositório original mude

## Conteúdo

```
fiware/
├── docker-compose.yml      → stack completo (6 containers)
├── mosquitto/
│   └── mosquitto.conf      → config do broker MQTT
├── LICENSE                  → MIT License (Fabio Cabrini)
└── README.md                → este arquivo
```

## Como subir

Dentro da VM Ubuntu (Azure, AWS, GCP ou local), com Docker e Docker Compose v2 instalados:

```bash
cd fiware
sudo docker compose up -d
```

### Subindo no Windows (Docker Desktop)

Se você está rodando localmente no Windows, troque os comandos `sudo docker` por `docker`:

```powershell
cd fiware
docker compose up -d
```

E para o ESP32 do Wokwi conseguir falar com o Mosquitto, você precisa expor a porta 1883 via **ngrok** (veja a seção "Alternativa: rodar localmente no Windows" no README principal).

### Confirmar que os containers subiram

```bash
sudo docker ps
```

Deve listar:

- `fiware-orion`
- `fiware-iot-agent`
- `fiware-sth-comet`
- `fiware-mosquitto`
- `fiware-mongo-historical`
- `fiware-mongo-internal`

## Como derrubar

```bash
sudo docker compose down
```

Os dados ficam preservados nos volumes Docker. Para começar do zero (apaga tudo!):

```bash
sudo docker compose down
sudo docker volume rm fiware_mongo-historical-data
sudo docker volume rm fiware_mongo-internal-data
```

## Portas usadas

| Porta | Serviço          | Abrir no firewall? |
|-------|------------------|--------------------|
| 1026  | Orion            | Sim                |
| 1883  | Mosquitto (MQTT) | Sim                |
| 4041  | IoT Agent UL     | Sim                |
| 8666  | STH-Comet        | Sim                |
| 9001  | Mosquitto WebSocket | Opcional (pro dashboard) |
| 27017 | MongoDB          | **Não** — risco de segurança |

No Azure, configure as inbound rules do Network Security Group para abrir essas portas.

## Health Check

Depois de subir, valide que tudo responde:

```bash
# Dentro da VM
curl http://localhost:1026/version    # Orion
curl http://localhost:8666/version    # STH-Comet
curl http://localhost:4041/iot/about  # IoT Agent

# Do navegador (substitua pelo IP da VM)
http://SEU_IP_DA_VM:1026/version
http://SEU_IP_DA_VM:8666/version
```

Se os três retornarem JSON com versão, o FIWARE está saudável.

## Configuração da Smart Lamp (PoC do Prof. Fábio)

A coleção Postman original do Prof. Fábio cobre o exemplo da "Smart Lamp". No OrbitStock essa PoC foi substituída pelo modelo do **compartimento M-03 da Cápsula Dragon** — veja a pasta `postman/` do projeto para a coleção adaptada.

## Documentação avançada

- FIWARE Step-by-step: https://fiware-tutorials.readthedocs.io
- Orion Context Broker: https://fiware-orion.readthedocs.io
- STH-Comet: https://fiware-sth-comet.readthedocs.io
- IoT Agent UL: https://fiware-iotagent-ul.readthedocs.io
