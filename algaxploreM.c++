#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Configuração do LCD
LiquidCrystal_I2C lcd(0x27, 20, 4); // Endereço I2C, colunas e linhas do LCD

// Pinos dos potenciômetros
const int energyPin = 16;  // Potenciômetro 1 (Energia)
const int carbonPin = 4;   // Potenciômetro 2 (CO2)
const int phPin = 0;       // Potenciômetro 3 (pH)

// Configuração Wi-Fi
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";

// Configuração MQTT
const char* mqtt_server = "SEU_BROKER_MQTT";
const int mqtt_port = 1883;
const char* mqtt_topic = "fiware/iot/sensors";

// Cliente Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  // Inicializa a comunicação I2C nos pinos especificados (SDA: 18, SCL: 19)
  Wire.begin(18, 19); // SDA: 18, SCL: 19

  // Inicializa o LCD
  lcd.begin(20, 4);
  lcd.backlight();

  // Inicializa a comunicação serial
  Serial.begin(115200);

  // Configura os pinos dos potenciômetros como entrada
  pinMode(energyPin, INPUT);
  pinMode(carbonPin, INPUT);
  pinMode(phPin, INPUT);

  // Conecta ao Wi-Fi
  connectToWiFi();

  // Configura o cliente MQTT
  client.setServer(mqtt_server, mqtt_port);
}

void connectToWiFi() {
  Serial.print("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  lcd.setCursor(0, 3);
  lcd.print("Wi-Fi conectado");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void readSensors(float &energy, float &carbon, float &ph) {
  int energyValue = analogRead(energyPin);
  int carbonValue = analogRead(carbonPin);
  int phValue = analogRead(phPin);

  energy = map(energyValue, 0, 4095, 50, 150);
  carbon = map(carbonValue, 0, 4095, 10, 500) / 10.0;
  ph = map(phValue, 0, 4095, 60, 85) / 10.0;
}

void displayLCD(float energy, float carbon, float ph) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Energia: ");
  lcd.print(energy);
  lcd.print(" kWh");
  lcd.setCursor(0, 1);
  lcd.print("CO2: ");
  lcd.print(carbon);
  lcd.print(" mg/L");
  lcd.setCursor(0, 2);
  lcd.print("pH: ");
  lcd.print(ph);
  lcd.setCursor(0, 3);
  lcd.print("Enviando MQTT...");
}

void sendMQTT(float energy, float carbon, float ph) {
  char payload[100];
  snprintf(payload, sizeof(payload), 
           "{\"energy\":%.2f,\"co2\":%.2f,\"ph\":%.2f}", 
           energy, carbon, ph);
  client.publish(mqtt_topic, payload);
  Serial.print("Publicado: ");
  Serial.println(payload);
}

void simulate_data() {
  float energy_generated, carbon_content, ph_level;
  readSensors(energy_generated, carbon_content, ph_level);

  // Exibir os dados no LCD
  displayLCD(energy_generated, carbon_content, ph_level);

  // Enviar os dados ao broker MQTT
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  sendMQTT(energy_generated, carbon_content, ph_level);
}

void loop() {
  simulate_data();
  delay(10000);
}
