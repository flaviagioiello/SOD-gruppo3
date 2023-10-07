// Codice per la realizzazione dei vari task dell'ESP32 e comunicazione via mqtt

// Librerie
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP280.h>
#include <ArduinoJson.h>
#include "RTClib.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mutex>
#include <atomic>


#define SCREEN_WIDTH 128 // Definizione larghezza display 
#define SCREEN_HEIGHT 64 // Definizione altezza display
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D // Indirizzo I2C display OLED
#define QUEUE_SIZE 100

// Dichiarazione dell'array per la coda dei messaggi
char jsonQueue[QUEUE_SIZE][500];
int queueFront = 0;
int queueRear = 0;

// Dichiarazione dei semafori per le risorse condivise
std::mutex m;
std::mutex s;
std::mutex fanSpeedMutex; 

// Imposta le credenziali Wifi
const char* ssid = "adelio"; // SSID rete Wi-Fi
const char* password = "ciaociao"; //Password rete Wi-Fi

// Imposta gli indirizzi del broker MQTT e la porta
const char* mqttServer = "192.168.113.229"; // Sostituisci con l'indirizzo IP della tua Raspberry Pi
const int mqttPort = 1883; // Porta del broker MQTT
const char* mqtt_topic = "orario"; // Topic MQTT per l'orario

// Imposta il client Wifi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Schermo oled e sensore pressione
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Oggetto per il display OLED
Adafruit_BMP280 bmp280; // Oggetto per il sensore BMP280
RTC_PCF8523 rtc; // Oggetto per il modulo RTC



char payload[200]; // Variabile globale per memorizzare il payload MQTT

int hours, minutes, seconds; // Variabili per memorizzare l'orario

// Dichiarazione globale di fanSpeed
int fanSpeed;

// Dichiarazione globale di rpm
volatile unsigned long rpm = 0;

const int tachPin = 16; 
volatile unsigned long pulseCount = 0;

// Decodifica il messaggio MQTT dalla Raspberry
void callback(char* topic, byte* payload, unsigned int length) {
  
  payload[length] = '\0'; // Assicura che la stringa sia terminata correttamente
  
  if (sscanf((char*)payload, "%d:%d:%d", &hours, &minutes, &seconds) == 3) {
    // Imposta solo l'orario RTC con i valori ottenuti
    rtc.adjust(DateTime(0, 0, 0, hours, minutes, seconds));
  }
  client.publish("Conferma","Orario ricevuto con successo"); // Invia una conferma
}

// Dichiarazione delle funzioni per i task
void taskWiFi_MQTT(void *pvParameters);
void taskSensors(void *pvParameters);
void taskFanControl(void *pvParameters);
void taskJSONPublish(void *pvParameters);



// Definizione del task di connessione MQTT
void taskWiFi_MQTT(void *pvParameters) {
  // Connessione WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(pdMS_TO_TICKS(5000));
      Serial.println("Connessione alla rete Wi-Fi...");
    }
    Serial.println("Connesso alla rete Wi-Fi");  // Verifica la connessione WiFi ogni 5 secondi
    client.setServer(mqttServer, 1883);
   // In attesa dell'orario via mqtt
    client.setCallback(callback);
    client.subscribe(mqtt_topic);
    //vTaskDelay(pdMS_TO_TICKS(5000));
  while (true) {
    if (!client.connected()){
    //vTaskDelay(pdMS_TO_TICKS(5000));
    reconnect();
     
     // Esegui il loop del client MQTT ogni 100 ms
  }
  client.loop();
  
}
}

// Funzione leggi dati dai sensori
void leggiBmp280(float &a,float &b){
 m.lock(); // blocca accesso alle risorse
     a = bmp280.readTemperature();
     b = bmp280.readPressure();
 m.unlock(); // sblocca accesso alle risorse
}

// Funzione per ottenere il valore di fanSpeed
unsigned int getPWM(){
  int result = 0;

  // Blocco per evitare l'accesso concorrente a fanSpeed
  fanSpeedMutex.lock();
  result = fanSpeed;
  fanSpeedMutex.unlock();

  return result;

}


// Funzione per ottenere il valore RPM
unsigned long getRPM() {
  unsigned long currentRPM;
  noInterrupts();
  currentRPM = rpm;
  interrupts();
  return currentRPM;
}

// Task per la visualizzazione sul display OLED
void taskSensors(void *pvParameters) {
  
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS); // Inizializza display oled
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  float temp=0.0;
  float press=0.0;
  while (true) {
    // Lettura dei sensori e aggiornamento dei valori
    display.clearDisplay();
    leggiBmp280(temp,press);
    display.setCursor(10, 10);
    display.println("Temp: " + String(temp) + " C");
    display.setCursor(10, 20);
    display.println("Press: " + String(press) + " Pa");
    display.setCursor(10, 30);
    display.println("PWM: " + String((getPWM() / 255.0) * 100.0) + " %");
    display.setCursor(10, 40);
    display.println("RPM: " +  String(getRPM()));
    display.display();
    
    //delay(1000);
    vTaskDelay(pdMS_TO_TICKS(5000)); // Lettura dei sensori ogni 5 secondi
  }
}

// Task per il controllo della ventola
void taskFanControl(void *pvParameters) {
  const int fanPin = 17; // Imposta il pin per il controllo della ventola
  const int feedbackPin = 32; // Imposta il pin di feedback della ventola
  const float targetTemp = 20.8; // Temperatura desiderata in gradi Celsius
  const float tempTolerance = 0.4; // Tolleranza di temperatura in gradi Celsius
  const int maxFanSpeed = 255; // Velocità massima della ventola (0-255)
  
  const int PWM_min = 0;
  const int PWM_max = 255;
  const int Velocita_min = 0;
  const int Velocita_max = 5000;
  
  pinMode(feedbackPin, INPUT_PULLUP);
  pinMode(fanPin, OUTPUT);

  while (true) {
    float currentTemp = bmp280.readTemperature(); // Leggi la temperatura attuale dal sensore
    float tempDifference = currentTemp-targetTemp;

    // Calcola la velocità della ventola in base alla differenza di temperatura
    //fanSpeed = map(tempDifference, -tempTolerance, tempTolerance, 0, maxFanSpeed);
    if (tempDifference > tempTolerance) {
     // Temperatura troppo alta, calcola la velocità in base alla differenza di temperatura
     fanSpeed = map(tempDifference, 0, 3, 20, maxFanSpeed);
     fanSpeed = constrain(fanSpeed, 0, maxFanSpeed);
     }   
    else if (tempDifference < -tempTolerance) {
     // Temperatura troppo bassa, disattiva gradualmente la ventola
     fanSpeed = map(tempDifference, -tempTolerance, 0, maxFanSpeed, 0);
     fanSpeed = constrain(fanSpeed, 0, maxFanSpeed);
     } 
    else {
     // Temperatura nella soglia di tolleranza, mantiene la velocità attuale
      }
    //fanSpeed = constrain(fanSpeed, 0, maxFanSpeed); // Assicura che la velocità sia nell'intervallo corretto


    analogWrite(fanPin, fanSpeed);

    
    vTaskDelay(pdMS_TO_TICKS(1000)); // Controllo della ventola ogni secondo
   }
 }

// Task per la creazione e l'invio del file JSON alla Raspberry
void taskJSONPublish(void *pvParameters) {
  StaticJsonDocument<200> doc ;
  char output[500];

 // Invio dei dati JSON tramite MQTT
  while (true) {
    doc.clear();
    // Acquisisce l'orario corrente
    DateTime now = rtc.now();
    char formattedTime[10];
    snprintf(formattedTime, sizeof(formattedTime), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    // Crea un oggetto JSON e aggiunge i dati
    doc["time"] = formattedTime;

    float temp = 0.0;
    float press = 0.0;
    leggiBmp280(temp, press);

    doc["temperatura"] = temp;
    doc["pressione"] = press;
    doc["Potenza della ventola"] = String((getPWM() / 255.0) * 100.0) + " %";
    doc["RPM"] = getRPM();

    serializeJson(doc, output);

    if (client.connected()) {
      // Invia il JSON se c'è connessione
      serializeJson(doc, output);
      client.publish("parametri", output);
      Serial.println("JSON inviato: ");
      Serial.print(output);
    } else {
      // La connessione MQTT non è disponibile, quindi aggiungi il JSON alla coda
      if (((queueRear + 1) % QUEUE_SIZE) != queueFront) {
        strcpy(jsonQueue[queueRear], output);
        queueRear = (queueRear + 1) % QUEUE_SIZE;
        Serial.println("JSON aggiunto alla coda: ");
        Serial.print(output);
        
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // Invio dei dati JSON ogni 5 secondi
  }
}


void countPulse() {
  pulseCount++;
}

// Task calcolo RPM ventola
void readRPMTask(void *pvParameters) {
  (void)pvParameters;

  const unsigned long interval = 1000; // Intervallo di lettura in millisecondi
  unsigned long previousMillis = 0;

  while (true) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      noInterrupts();
      unsigned long currentPulseCount = pulseCount;
      pulseCount = 0; // Resetta il conteggio degli impulsi
      interrupts();

      rpm = (currentPulseCount * 60000UL) / (interval * 2UL); // Il sensore dà due impulsi per ogni rotazione completa

      previousMillis = currentMillis;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Attende 100 ms prima di eseguire un nuovo controllo
  }
}

void setup() {
  
  Wire.begin();
  Serial.begin(115200);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1) delay(10);
  }
  if (! rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!"); 
  }
  rtc.start();
  boolean status = bmp280.begin(0x77); // Inizializza sensore
  pinMode(tachPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(tachPin), countPulse, RISING);

  // Creazione dei task
  xTaskCreatePinnedToCore(readRPMTask,"readRPMTask", 4000, NULL, 3, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(taskWiFi_MQTT, "WiFi_MQTT Task", 4000, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(taskSensors, "Sensors Task", 4000, NULL, 3, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(taskFanControl, "Fan Control Task", 4000, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(taskJSONPublish, "JSON Publish Task", 4000, NULL, 4, NULL, ARDUINO_RUNNING_CORE);
  
}


void loop() {
  // Non utilizzato quando si utilizza FreeRTOS
}

void reconnect() {
  // Loop fino a quando non ci si è riconnessi al broker MQTT
  while (!client.connected()) {
    Serial.println("Connessione al broker MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connesso al broker MQTT"); 

      // La connessione è stata ripristinata, invia i messaggi dalla coda
      while (queueFront != queueRear) {
        vTaskDelay(pdMS_TO_TICKS(500));
        client.publish("parametri", jsonQueue[queueFront]);
        Serial.println("JSON della coda inviato: ");
        Serial.print(jsonQueue[queueFront]);
        queueFront = (queueFront + 1) % QUEUE_SIZE;

      }

      client.subscribe(mqtt_topic); // Sottoscrivi al topic solo se la connessione è stata stabilita
    } else {
      Serial.print("Errore di connessione, rc=");
      Serial.print(client.state());
      Serial.print(" Riprovo tra 5 secondi...");
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}
