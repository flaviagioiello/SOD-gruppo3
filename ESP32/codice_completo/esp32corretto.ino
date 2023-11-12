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

std::mutex m;
std::mutex s;
std::mutex fanSpeedMutex; // Dichiarazione del mutex per fanSpeed

//Imposta le credenziali Wifi
const char* ssid = "adelio"; // SSID rete Wi-Fi
const char* password = "ciaociao"; //Password rete Wi-Fi

//Imposta gli indirizzi del broker MQTT e la porta
const char* mqttServer = "192.168.171.229"; // Sostituisci con l'indirizzo IP del tuo Raspberry Pi
const int mqttPort = 1883; // Porta del broker MQTT
const char* mqtt_topic = "orario"; // Topic MQTT per l'orario

//Imposta il client Wifi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// schermo oled e sensore pressione
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Oggetto per il display OLED
Adafruit_BMP280 bmp280; // Oggetto per il sensore BMP280
RTC_PCF8523 rtc; // Oggetto per il modulo RTC



char payload[200]; // Variabile globale per memorizzare il payload MQTT

int hours, minutes, seconds; // Variabili per memorizzare l'orario
TickType_t xLastWakeTime1;
TickType_t xLastWakeTime2;
TickType_t xLastWakeTime3;
TickType_t xLastWakeTime4;

void callback(char* topic, byte* payload, unsigned int length) {
  // Decodifica il messaggio MQTT
  payload[length] = '\0'; // Assicurati che la stringa sia terminata correttamente
  
  if (sscanf((char*)payload, "%d:%d:%d", &hours, &minutes, &seconds) == 3) {
    // Imposta solo l'orario RTC con i valori ottenuti
    rtc.adjust(DateTime(0, 0, 0, hours, minutes, seconds));
    
    // Invia una conferma di successo
    client.publish("Conferma", "Orario ricevuto con successo");
  }
}

// Dichiarazione delle funzioni per i task
void taskWiFi_MQTT(void *pvParameters);
void taskSensors(void *pvParameters);
void taskFanControl(void *pvParameters);
void taskJSONPublish(void *pvParameters);


// Definizione dei task
void taskWiFi_MQTT(void *pvParameters) {
  // Connessione WiFi
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(5000));
    Serial.println("Connessione alla rete Wi-Fi...");
  }
  
  Serial.println("Connesso alla rete Wi-Fi");  // Verifica la connessione WiFi ogni 5 secondi
  client.setServer(mqttServer, 1883);

  // In attesa dell'orario via MQTT
  client.setCallback(callback);
  client.subscribe(mqtt_topic);
  
  while (true) {
    if (!client.connected()) {
      reconnect();
    }
    
    // Esegui il loop del client MQTT ogni 100 ms
    client.loop();
  }
}


void leggiBmp280(float &a,float &b){

     a = bmp280.readTemperature();
     b = bmp280.readPressure();
 
}

// Dichiarazione globale di fanSpeed
int fanSpeed;

unsigned int getPWM(){
  int result = 0;

  // Blocco per evitare l'accesso concorrente a fanSpeed
  fanSpeedMutex.lock();
  result = fanSpeed;
  fanSpeedMutex.unlock();

  return result;

}

// Dichiarazione globale di rpm
volatile unsigned long rpm = 0;

unsigned long getRPM() {
  unsigned long currentRPM;
  noInterrupts();
  currentRPM = rpm;
  interrupts();
  return currentRPM;
}

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
    vTaskDelayUntil(&xLastWakeTime1,5000 / portTICK_PERIOD_MS); // Lettura dei sensori ogni 5 secondi
  }
}

void taskFanControl(void *pvParameters) {
  const int fanPin = 17; // Imposta il pin per il controllo della ventola

  const float targetTemp = 17.4; // Temperatura desiderata in gradi Celsius
  const float tempTolerance = 0.4; // Tolleranza di temperatura in gradi Celsius
  const int maxFanSpeed = 255; // Velocità massima della ventola (0-255)
  
  const int PWM_min = 0;
  const int PWM_max = 255;
  const int Velocita_min = 0;
  const int Velocita_max = 5000;
  
  pinMode(fanPin, OUTPUT);

  while (true) {
    float currentTemp = bmp280.readTemperature(); // Leggi la temperatura attuale dal tuo sensore
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
     // Temperatura nella soglia di tolleranza, mantieni la velocità attuale
      }
    //fanSpeed = constrain(fanSpeed, 0, maxFanSpeed); // Assicurati che la velocità sia nell'intervallo corretto


    analogWrite(fanPin, fanSpeed);

    
    vTaskDelayUntil(&xLastWakeTime2,1000 / portTICK_PERIOD_MS); // Controllo della ventola ogni secondo
   }
 }

void taskJSONPublish(void *pvParameters) {
  StaticJsonDocument<200> doc ;
  char output[500];

 

 
  while (true) {
    doc.clear();
    // Acquisisci l'orario corrente
    DateTime now = rtc.now();
    char formattedTime[10];
    snprintf(formattedTime, sizeof(formattedTime), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    // Crea un oggetto JSON e aggiungi i dati
    StaticJsonDocument<200> doc;
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
      // Invia il JSON
      serializeJson(doc, output);
      client.publish("parametri", output);
      Serial.println("JSON inviato: ");
      Serial.println(output);
    } else {
      // La connessione MQTT non è disponibile, quindi aggiungi il JSON alla coda
      if (((queueRear + 1) % QUEUE_SIZE) != queueFront) {
        strcpy(jsonQueue[queueRear], output);
        queueRear = (queueRear + 1) % QUEUE_SIZE;
        Serial.println("JSON aggiunto alla coda: ");
        Serial.println(output);

        
      }
    }

    vTaskDelayUntil(&xLastWakeTime3,5000 / portTICK_PERIOD_MS); // Invio dei dati JSON ogni 5 secondi
  }
}


const int tachPin = 16; 
volatile unsigned long pulseCount = 0;

void countPulse() {
  pulseCount++;
}

void readRPMTask(void *pvParameters) {
  (void)pvParameters;

  while (true) {
    noInterrupts();
    unsigned long currentPulseCount = pulseCount;
    pulseCount = 0; // Resetta il conteggio degli impulsi
    interrupts();
    rpm = (currentPulseCount * 60000UL) / (1000 * 2UL); // Il sensore dà due impulsi per ogni rotazione completa
    vTaskDelay(pdMS_TO_TICKS(1000)); // Attendi un po' prima di eseguire un nuovo controllo
  }
}

void setup() {
  xLastWakeTime1 = xTaskGetTickCount();  
  xLastWakeTime2 = xTaskGetTickCount();
  xLastWakeTime3 = xTaskGetTickCount();
  xLastWakeTime4 = xTaskGetTickCount();
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
  boolean status = bmp280.begin(0x77); //inizializza sensore
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
        Serial.println(jsonQueue[queueFront]);
        queueFront = (queueFront + 1) % QUEUE_SIZE;

      }

      client.subscribe(mqtt_topic); // Sottoscrivi al topic solo se la connessione è stata stabilita
    } else {
      Serial.print("Errore di connessione, rc=");
      Serial.println(client.state());
      Serial.println(" Riprovo tra 5 secondi...");
      vTaskDelayUntil(&xLastWakeTime4 ,5000 / portTICK_PERIOD_MS);
    }
  }
}