
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ===== WLAN-Zugangsdaten =====
const char* WIFI_SSID = "";
const char* WIFI_PASSWORD = "";

// ===== Home Assistant / Mosquitto =====
// Hier die lokale IP deiner Home-Assistant-VM eintragen!
const char* MQTT_SERVER = "";
const int MQTT_PORT = ;

// MQTT-Benutzer und Passwort für Mosquitto
const char* MQTT_USER = "";
const char* MQTT_PASSWORD = "";

// MQTT-Topics: passend zu deinen HA-Automationen
const char* TOPIC_LIGHT =
    "smartknob/command/light/set";

const char* TOPIC_TEMPERATURE =
    "smartknob/command/temperatur/set";

const char* TOPIC_MODE =
    "smartknob/mode";

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// ===== Smart-Knob-Zustand =====
enum Mode {
    LIGHT_MODE,
    HEATING_MODE
};

Mode currentMode = LIGHT_MODE;

// Werte bleiben beim Moduswechsel separat erhalten
int lightLevel = 10;

const int temperatureSteps[] = {
    18, 20, 22, 24, 26, 28
};

const int numberOfTemperatureSteps = 6;
int temperatureIndex = 0;

// ===== WLAN verbinden =====
void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.print("Verbinde mit WLAN...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWLAN verbunden!");
    Serial.print("ESP32-IP: ");
    Serial.println(WiFi.localIP());
}

// ===== MQTT verbinden =====
void connectMQTT() {
    while (!mqtt.connected()) {
        Serial.print("Verbinde mit MQTT...");

        if (mqtt.connect(
                "SmartKnobESP32",
                MQTT_USER,
                MQTT_PASSWORD)) {
            Serial.println("verbunden!");
            //publishMode();
        } else {
            Serial.print("Fehler, Status: ");
            Serial.println(mqtt.state());
            delay(3000);
        }
    }
}

// ===== Modus veröffentlichen =====
void publishMode() {
    const char* modeText =
        (currentMode == LIGHT_MODE)
        ? "Beleuchtung"
        : "Heizung";

    mqtt.publish(TOPIC_MODE, modeText, true);
    Serial.print("Modus: ");
    Serial.println(modeText);
}

// ===== Lichtwert an HA senden =====
void publishLight() {
    char payload[8];
    snprintf(payload, sizeof(payload), "%d", lightLevel);

    if (mqtt.publish(TOPIC_LIGHT, payload)) {
        Serial.print("Lichtwert gesendet: ");
        Serial.print(lightLevel);
        Serial.println(" %");
    } else {
        Serial.println("Fehler beim Senden des Lichtwerts.");
    }
}

// ===== Temperatur an HA senden =====
void publishTemperature() {
    char payload[8];
    int temperature = temperatureSteps[temperatureIndex];

    snprintf(payload, sizeof(payload), "%d", temperature);

    if (mqtt.publish(TOPIC_TEMPERATURE, payload)) {
        Serial.print("Temperatur gesendet: ");
        Serial.print(temperature);
        Serial.println(" °C");
    } else {
        Serial.println("Fehler beim Senden der Temperatur.");
    }
}

// ===== Drehung simulieren =====
void rotateKnob(int direction) {
    if (currentMode == LIGHT_MODE) {
        int newLevel = lightLevel + direction * 10;

        // Grenzen: 10 bis 100 %, in 10er-Schritten
        if (newLevel < 10 || newLevel > 100) {
            Serial.println("Licht: Grenzwert erreicht.");
            return;
        }

        lightLevel = newLevel;
        publishLight();

    } else {
        int newIndex = temperatureIndex + direction;

        // Grenzen: 18 bis 28 °C, in 2er-Schritten
        if (newIndex < 0 ||
            newIndex >= numberOfTemperatureSteps) {
            Serial.println("Heizung: Grenzwert erreicht.");
            return;
        }

        temperatureIndex = newIndex;
        publishTemperature();
    }
}

// ===== Modus umschalten =====
void toggleMode() {
    currentMode =
        (currentMode == LIGHT_MODE)
        ? HEATING_MODE
        : LIGHT_MODE;

    publishMode();

    if (currentMode == LIGHT_MODE) {
        Serial.print("Aktueller Lichtwert: ");
        Serial.print(lightLevel);
        Serial.println(" %");
    } else {
        Serial.print("Aktuelle Temperatur: ");
        Serial.print(temperatureSteps[temperatureIndex]);
        Serial.println(" °C");
    }
}

// ===== Setup =====
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== Smart Knob startet ===");

    connectWiFi();

    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    connectMQTT();

    Serial.println("\nSteuerung zum Testen:");
    Serial.println("+ = Knopf im Uhrzeigersinn drehen");
    Serial.println("- = Knopf gegen den Uhrzeigersinn drehen");
    Serial.println("m = Knopf drücken / Modus wechseln");
    Serial.println("================================");
}

// ===== Hauptschleife =====
void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (!mqtt.connected()) {
        connectMQTT();
    }

    mqtt.loop();

    // Temporäre Simulation eines Drehgebers
    while (Serial.available() > 0) {
        char command = Serial.read();

        if (command == '+') {
            rotateKnob(1);
        } else if (command == '-') {
            rotateKnob(-1);
        } else if (command == 'm' || command == 'M') {
            toggleMode();
        }
        // Zeilenumbrüche im Serial Monitor ignorieren
    }
}


