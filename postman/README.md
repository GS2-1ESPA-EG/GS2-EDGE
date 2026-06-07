# Postman — Coleção FIWARE OrbitStock

Esta pasta contém a coleção Postman customizada do OrbitStock, com todas as requisições necessárias para provisionar e operar o FIWARE.

## Arquivos

- **`OrbitStock.postman_collection.json`** — coleção principal
- **`OrbitStock-FIWARE.postman_environment.json`** — environment com a variável `{{url}}`

## Como importar

1. Abra o Postman.
2. No canto superior esquerdo, **Collections → Import**.
3. Arraste os dois arquivos `.json` desta pasta.
4. Vá em **Environments**, abra **OrbitStock-FIWARE**, e ajuste a variável `url`:

| Cenário | Valor de `url` |
|---------|----------------|
| **FIWARE na Azure** (padrão do projeto) | `158.23.61.7` (ou o IP público da sua VM) |
| **FIWARE local no PC** (Docker Desktop) | `localhost` |

> 💡 Quando você usa `localhost`, as requisições do Postman vão para o FIWARE rodando no seu PC. Isso é independente do ESP32 do Wokwi (que precisa do ngrok pra alcançar o seu PC pela porta MQTT). O Postman, sendo um app desktop, fala direto com seus containers locais — sem ngrok.

5. No canto superior direito do Postman, selecione **OrbitStock-FIWARE** como environment ativo.

## Organização da coleção

```
OrbitStock - Edge Computing/
├── 1. Health Check/
│   ├── 1.1 Orion Version
│   ├── 1.2 IoT Agent Version
│   └── 1.3 STH-Comet Version
├── 2. Provisionamento (uma vez)/
│   ├── 2.1 Criar Service Group (UL /iot/d)
│   ├── 2.2 Listar Service Groups
│   ├── 2.3 Criar Device compartment_m03
│   ├── 2.4 Listar Devices
│   └── 2.5 Criar Entidade no Orion (snapshot inicial)
├── 3. Consultas (uso diario)/
│   ├── 3.1 Estado atual do compartimento M-03
│   └── 3.2 Listar todas as entidades
├── 4. Comandos cloud -> edge (testar atuadores)/
│   ├── 4.1 Acionar ALERT (LED + buzzer ligados)
│   └── 4.2 Acionar ACK (desliga atuadores)
└── 5. Manutencao (limpeza em caso de erro)/
    ├── 5.1 Deletar Device
    ├── 5.2 Deletar Service Group
    ├── 5.3 Deletar Entidade no Orion
    └── 5.4 Deletar Entidade fantasma
```

## Ordem recomendada (primeira execução)

1. **1.1, 1.2, 1.3** → confirma que o FIWARE está respondendo
2. **2.1** → cria o Service Group
3. **2.3** → cria o Device (vincula ao Orion)
4. **2.5** → cria a Entidade no Orion (snapshot inicial)
5. **2.2** e **2.4** → confirma o provisionamento
6. Ligue o Wokwi
7. **3.1** → veja a entidade sendo atualizada com os valores do ESP32 (a cada 5s)

## Ordem recomendada (testar comandos)

1. Wokwi rodando (LED apagado)
2. **4.1** → comando `alert` sai do Postman, viaja Orion → IoT Agent → MQTT → ESP32, e o LED acende
3. **4.2** → comando `ack` desliga o LED

## Resetar tudo (em caso de problema)

**IMPORTANTE:** pare o Wokwi primeiro (botão ⏹), senão o auto-provisioning do IoT Agent recria devices/entidades automaticamente.

1. **5.1** → deletar Device
2. **5.4** → deletar Entidade fantasma se existir
3. **5.2** → deletar Service Group
4. **5.3** → deletar Entidade do Orion (opcional, se quiser recomeçar do zero)

Depois reexecute **2.1, 2.3, 2.5** na ordem.
