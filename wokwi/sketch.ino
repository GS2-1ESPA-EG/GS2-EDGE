/*
 * ============================================================
 *  OrbitStock - Sistema de gestao de carga para Capsula Dragon
 *  Entrega Edge Computing - FIAP Global Solution 2026
 * ============================================================
 *
 *  Autores:
 *    - Enrico Dellatorre  RM566824
 *    - Gustavo Hiruo      RM567625
 *
 *  Disciplina: Edge Computing & Computer Systems
 *  Turma     : 1ESPA - Engenharia de Software
 *  Data      : Maio/2026
 *
 *  Descricao:
 *    Firmware do ESP32 simulado no Wokwi que representa o
 *    sistema embarcado de monitoramento do compartimento M-03
 *    (medico refrigerado) da Capsula Dragon C209.
 *
 *    Publica telemetria MQTT (Ultralight 2.0) para o FIWARE
 *    IoT Agent UL hospedado na VM Azure, e assina o topico
 *    de comando para receber acoes do centro de controle.
 *
 *  Pipeline:
 *    ESP32 -> MQTT -> Mosquitto -> IoT Agent UL -> Orion
 * ============================================================
 */

#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// =====================================================
// CORES ANSI (renderizadas pelo Serial Monitor do Wokwi)
// =====================================================
#define ANSI_RESET   "\033[0m"
#define ANSI_BOLD    "\033[1m"
#define ANSI_DIM     "\033[2m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_WHITE   "\033[37m"
#define ANSI_GRAY    "\033[90m"
#define ANSI_BG_RED  "\033[41m"

// =====================================================
// CONFIGURACAO DE REDE E FIWARE
// =====================================================
const char* WIFI_SSID   = "Wokwi-GUEST";
const char* WIFI_PASS   = "";

const char* MQTT_BROKER = "158.23.61.7";              // IP publico da VM Azure
const int   MQTT_PORT   = 1883;
const char* API_KEY     = "4jggokgpepnvsb2uv4s40d59ov";
const char* DEVICE_ID   = "compartment_m03";

char TOPIC_PUB[80];
char TOPIC_CMD[80];

// =====================================================
// IDENTIDADE LOGICA
// =====================================================
const char* SPACECRAFT_ID    = "DRAGON-C209";
const char* MISSION_ID       = "CRS-31";
const char* COMPARTMENT_ID   = "M-03";
const char* COMPARTMENT_NAME = "Medico refrigerado";

// =====================================================
// PINAGEM
// =====================================================
#define PIN_DHT     15
#define PIN_LED      2
#define PIN_BUZZER   4
#define PIN_LDR     34

// =====================================================
// FAIXA OPERACIONAL DO COMPARTIMENTO M-03
// =====================================================
const float TEMP_MIN          = 2.0;
const float TEMP_MAX          = 8.0;
const float HUMIDITY_MAX      = 80.0;
const float IMPACT_G_MAX      = 2.0;
const int   LDR_LIMIAR_ABERTURA = 2000;
const unsigned long INTERVALO_LEITURA_MS = 5000;

// =====================================================
// OBJETOS GLOBAIS
// =====================================================
DHT dht(PIN_DHT, DHT22);
Adafruit_MPU6050 mpu;
WiFiClient espClient;
PubSubClient mqtt(espClient);

unsigned long ultimaLeitura = 0;
unsigned long contadorLeituras = 0;
unsigned long tempoInicio = 0;

// =====================================================
// HELPERS DE EXIBICAO
// =====================================================

// Barra ASCII [=====-----] colorida por severidade (0=verde, 1=amarelo, 2=vermelho)
void desenharBarra(float valor, float minVal, float maxVal, int severidade) {
  const int LARGURA = 10;
  float pct = (valor - minVal) / (maxVal - minVal);
  if (pct < 0) pct = 0;
  if (pct > 1) pct = 1;
  int preenchidos = (int)(pct * LARGURA + 0.5);

  const char* cor;
  switch (severidade) {
    case 2:  cor = ANSI_RED;    break;
    case 1:  cor = ANSI_YELLOW; break;
    default: cor = ANSI_GREEN;  break;
  }

  Serial.print("[");
  Serial.print(cor);
  for (int i = 0; i < preenchidos; i++) Serial.print("=");
  Serial.print(ANSI_GRAY);
  for (int i = preenchidos; i < LARGURA; i++) Serial.print("-");
  Serial.print(ANSI_RESET);
  Serial.print("]");
}

// T+HH:MM:SS desde o boot
void imprimirUptime() {
  unsigned long s = (millis() - tempoInicio) / 1000;
  unsigned long h = s / 3600; s %= 3600;
  unsigned long m = s / 60;   s %= 60;
  char buf[16];
  snprintf(buf, sizeof(buf), "T+%02lu:%02lu:%02lu", h, m, s);
  Serial.print(buf);
}

// Cabecalho do painel
void imprimirCabecalho() {
  Serial.println();
  Serial.print(ANSI_BOLD); Serial.print(ANSI_CYAN);
  Serial.println("============================================");
  Serial.print  ("  ORBITSTOCK | "); Serial.print(MISSION_ID);
  Serial.print  (" | "); Serial.print(SPACECRAFT_ID);
  Serial.print  (" | "); Serial.println(COMPARTMENT_ID);
  Serial.println("============================================");
  Serial.print(ANSI_RESET);
}

// =====================================================
// CALLBACK DE COMANDOS (cloud -> edge)
// =====================================================
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  Serial.println();
  Serial.print(ANSI_BOLD); Serial.print(ANSI_CYAN);
  Serial.println("  >> COMANDO RECEBIDO DO CENTRO DE CONTROLE");
  Serial.print(ANSI_RESET); Serial.print(ANSI_DIM);
  Serial.print("     topic: "); Serial.println(topic);
  Serial.print("     msg  : "); Serial.println(msg);
  Serial.print(ANSI_RESET);

  if (msg.indexOf("alert") >= 0) {
    digitalWrite(PIN_LED, HIGH);
    tone(PIN_BUZZER, 1500, 500);
    Serial.print(ANSI_BG_RED); Serial.print(ANSI_BOLD);
    Serial.print(" ALERTA ATIVADO REMOTAMENTE ");
    Serial.println(ANSI_RESET);
    Serial.println("     -> LED on, buzzer on");
  }
  if (msg.indexOf("ack") >= 0) {
    digitalWrite(PIN_LED, LOW);
    noTone(PIN_BUZZER);
    Serial.print(ANSI_GREEN); Serial.print(ANSI_BOLD);
    Serial.print(" ALERTA RECONHECIDO ");
    Serial.println(ANSI_RESET);
    Serial.println("     -> Atuadores desligados");
  }
  Serial.println();
}

// =====================================================
// CONEXAO WIFI
// =====================================================
void conectarWiFi() {
  Serial.print(ANSI_DIM); Serial.print("  Conectando ao WiFi ");
  Serial.print(WIFI_SSID); Serial.print(" ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.print(ANSI_RESET); Serial.print(ANSI_GREEN);
  Serial.print(" [OK]");
  Serial.print(ANSI_RESET); Serial.print(ANSI_DIM);
  Serial.print(" IP: "); Serial.println(WiFi.localIP());
  Serial.print(ANSI_RESET);
}

// =====================================================
// CONEXAO MQTT
// =====================================================
void conectarMQTT() {
  while (!mqtt.connected()) {
    Serial.print(ANSI_DIM);
    Serial.print("  Conectando ao FIWARE ");
    Serial.print(MQTT_BROKER); Serial.print(":"); Serial.print(MQTT_PORT);
    Serial.print(" ... ");

    String clientId = String(DEVICE_ID) + "-" + String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.print(ANSI_RESET); Serial.print(ANSI_GREEN);
      Serial.println("[OK]");
      Serial.print(ANSI_RESET); Serial.print(ANSI_DIM);
      Serial.print("  Inscrito em "); Serial.println(TOPIC_CMD);
      Serial.print(ANSI_RESET);
      mqtt.subscribe(TOPIC_CMD);
    } else {
      Serial.print(ANSI_RESET); Serial.print(ANSI_RED);
      Serial.print("[FAIL rc="); Serial.print(mqtt.state()); Serial.println("]");
      Serial.print(ANSI_RESET);
      delay(2000);
    }
  }
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  snprintf(TOPIC_PUB, sizeof(TOPIC_PUB), "/%s/%s/attrs", API_KEY, DEVICE_ID);
  snprintf(TOPIC_CMD, sizeof(TOPIC_CMD), "/%s/%s/cmd",   API_KEY, DEVICE_ID);

  imprimirCabecalho();
  Serial.print(ANSI_DIM);
  Serial.print("  Compartimento : "); Serial.print(COMPARTMENT_ID);
  Serial.print(" ("); Serial.print(COMPARTMENT_NAME); Serial.println(")");
  Serial.print("  Faixa segura  : "); Serial.print(TEMP_MIN, 1);
  Serial.print(" a "); Serial.print(TEMP_MAX, 1); Serial.println(" C");
  Serial.print("  Umidade max   : "); Serial.print(HUMIDITY_MAX, 0); Serial.println(" %");
  Serial.println("  --------------------------------------------");
  Serial.print(ANSI_RESET);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  dht.begin();
  Serial.print(ANSI_DIM);
  Serial.print("  Inicializando sensores ");
  if (!mpu.begin()) {
    Serial.print(ANSI_RESET); Serial.print(ANSI_RED);
    Serial.println("[MPU6050 FAIL]");
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    Serial.print(ANSI_RESET); Serial.print(ANSI_GREEN);
    Serial.println("[OK]");
  }
  Serial.print(ANSI_RESET);

  conectarWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  conectarMQTT();

  Serial.print(ANSI_BOLD); Serial.print(ANSI_GREEN);
  Serial.println();
  Serial.println("  >> SISTEMA OPERACIONAL - TELEMETRIA ATIVA <<");
  Serial.print(ANSI_RESET);
  Serial.println();

  tempoInicio = millis();
}

// =====================================================
// LOOP PRINCIPAL
// =====================================================
void loop() {
  if (!mqtt.connected()) conectarMQTT();
  mqtt.loop();

  if (millis() - ultimaLeitura < INTERVALO_LEITURA_MS) return;
  ultimaLeitura = millis();
  contadorLeituras++;

  // --- LEITURAS ---
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();
  if (isnan(temp)) temp = 5.0;
  if (isnan(umid)) umid = 50.0;

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);
  float impacto_g = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  ) / 9.81;

  static int picosSeguidos = 0;
  if (impacto_g > IMPACT_G_MAX) picosSeguidos++; else picosSeguidos = 0;
  bool impactoDetectado = picosSeguidos >= 2;

  int  luz = analogRead(PIN_LDR);
  bool aberto = luz > LDR_LIMIAR_ABERTURA;

  // --- CLASSIFICACAO POR ATRIBUTO ---
  int sevTemp = 0;
  if (temp > TEMP_MAX || temp < TEMP_MIN) sevTemp = 2;
  else if (temp > TEMP_MAX - 1.0 || temp < TEMP_MIN + 1.0) sevTemp = 1;

  int sevUmid = (umid > HUMIDITY_MAX) ? 1 : 0;
  int sevImp  = impactoDetectado ? 2 : (impacto_g > IMPACT_G_MAX * 0.8 ? 1 : 0);
  int sevLuz  = aberto ? 1 : 0;

  // --- SEVERIDADE GERAL ---
  const char* severity = "nominal";
  int sevGeral = 0;
  if (sevTemp == 2 || sevImp == 2) { severity = "critical"; sevGeral = 2; }
  else if (sevUmid == 1 || sevLuz == 1 || sevTemp == 1) { severity = "warning"; sevGeral = 1; }

  // --- ATUADORES LOCAIS ---
  if (sevGeral == 2) {
    digitalWrite(PIN_LED, HIGH);
    tone(PIN_BUZZER, 1500, 200);
  } else if (sevGeral == 1) {
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));
  } else {
    digitalWrite(PIN_LED, LOW);
    noTone(PIN_BUZZER);
  }

  // --- REIMPRIME CABECALHO A CADA 10 LEITURAS ---
  if (contadorLeituras > 1 && (contadorLeituras - 1) % 10 == 0) {
    imprimirCabecalho();
  }

  // --- PAINEL DA LEITURA ---
  Serial.println();
  Serial.print(ANSI_BOLD); Serial.print("[#");
  Serial.print(contadorLeituras); Serial.print("]  ");
  Serial.print(ANSI_RESET); Serial.print(ANSI_DIM);
  imprimirUptime();
  Serial.print("   STATUS=");
  Serial.print(mqtt.connected() ? "ONLINE" : "OFFLINE");
  Serial.print("  UPLINK=");
  Serial.println(WiFi.status() == WL_CONNECTED ? "OK" : "DOWN");
  Serial.print(ANSI_RESET);

  // TEMP
  Serial.print("  TEMP    ");
  desenharBarra(temp, TEMP_MIN, TEMP_MAX + 2.0, sevTemp);
  Serial.print("  ");
  if (sevTemp == 2) Serial.print(ANSI_RED);
  else if (sevTemp == 1) Serial.print(ANSI_YELLOW);
  else Serial.print(ANSI_GREEN);
  Serial.print(temp, 1); Serial.print(" C");
  Serial.print(ANSI_RESET); Serial.print(ANSI_GRAY);
  Serial.print("   (limite: "); Serial.print(TEMP_MAX, 1); Serial.println(" C)");
  Serial.print(ANSI_RESET);

  // UMIDADE
  Serial.print("  UMID    ");
  desenharBarra(umid, 0, 100, sevUmid);
  Serial.print("  ");
  if (sevUmid >= 1) Serial.print(ANSI_YELLOW); else Serial.print(ANSI_GREEN);
  Serial.print(umid, 1); Serial.print(" %");
  Serial.print(ANSI_RESET); Serial.print(ANSI_GRAY);
  Serial.print("   (limite: "); Serial.print(HUMIDITY_MAX, 0); Serial.println(" %)");
  Serial.print(ANSI_RESET);

  // IMPACTO
  Serial.print("  IMPACT  ");
  desenharBarra(impacto_g, 0, IMPACT_G_MAX * 1.5, sevImp);
  Serial.print("  ");
  if (sevImp == 2) Serial.print(ANSI_RED);
  else if (sevImp == 1) Serial.print(ANSI_YELLOW);
  else Serial.print(ANSI_GREEN);
  Serial.print(impacto_g, 2); Serial.print(" g");
  Serial.print(ANSI_RESET); Serial.print(ANSI_GRAY);
  Serial.print("   (seguro: <"); Serial.print(IMPACT_G_MAX, 1); Serial.println(" g)");
  Serial.print(ANSI_RESET);

  // LUMINOSIDADE
  Serial.print("  LUZ     ");
  desenharBarra(luz, 0, 4095, sevLuz);
  Serial.print("  ");
  if (sevLuz >= 1) Serial.print(ANSI_YELLOW); else Serial.print(ANSI_GREEN);
  Serial.print(luz);
  Serial.print(ANSI_RESET); Serial.print(ANSI_GRAY);
  Serial.print("    ("); Serial.print(aberto ? "ABERTO" : "fechado"); Serial.println(")");
  Serial.print(ANSI_RESET);

  // STATUS GERAL
  Serial.println();
  if (sevGeral == 2) {
    Serial.print(ANSI_BG_RED); Serial.print(ANSI_BOLD);
    Serial.print(" >> CRITICAL ");
    Serial.print(ANSI_RESET); Serial.print(ANSI_RED);
    if (sevTemp == 2) Serial.println(" - temperatura fora da faixa segura");
    else if (sevImp == 2) Serial.println(" - impacto acima do limite");
    Serial.println("     LED on, buzzer on, terra notificada");
    Serial.print(ANSI_RESET);
  } else if (sevGeral == 1) {
    Serial.print(ANSI_YELLOW); Serial.print(ANSI_BOLD);
    Serial.print(" >> WARNING");
    Serial.print(ANSI_RESET); Serial.print(ANSI_YELLOW);
    Serial.println(" - atencao recomendada");
    Serial.print(ANSI_RESET);
  } else {
    Serial.print(ANSI_GREEN); Serial.print(ANSI_BOLD);
    Serial.print(" >> NOMINAL");
    Serial.print(ANSI_RESET); Serial.print(ANSI_DIM);
    Serial.println(" - todos parametros normais");
    Serial.print(ANSI_RESET);
  }

  // PAYLOAD ULTRALIGHT
  char payload[200];
  snprintf(payload, sizeof(payload),
    "t|%.1f|h|%.1f|g|%.2f|l|%d|o|%s|s|%s",
    temp, umid, impacto_g, luz,
    aberto ? "true" : "false",
    severity);

  bool ok = mqtt.publish(TOPIC_PUB, payload);

  Serial.print(ANSI_DIM);
  Serial.print(" >> TX ");
  Serial.print(ok ? "[OK] " : "[FAIL] ");
  Serial.println(payload);
  Serial.print(ANSI_RESET);
}
