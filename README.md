# Sistemi Operativi Dedicati - Sistema di monitoraggio dei parametri ambientali (Progetto 3)
Questo progetto si basa sull’implementazione di un sistema IoT completo che
combina componenti hardware e software per acquisire, elaborare e condividere dati
ambientali in tempo reale, in grado di rispondere alle loro variazioni in tempo reale.

# Indice
1. [Materiali utilizzati](#materiali)
2. [Descrizione del progetto](#introduzione)
3. [Configurazione e setup](#conf)
    1. [Configurazione Hardware](#hardware)
       1. [Sensore BMP280](#bmp280)
       2. [Display OLED](#oled)
       3. [Modulo RTC](#rtc)
       4. [Ventola NOCTUA NF-A4x10 5V PWM](#ventola)
    2. [Setup Software](#software)
       1. [Setup Raspberry Pi 4](#raspberry)
       2. [Setup ESP32](#esp32)
       3. [Installazione delle librerie](#librerie)
4. [Guida al codice](#guida)
5. [Software utilizzati](#software)
6. [Autori](#autori)

***
## Materiali utilizzati <a name="materiali"></a>
* ESP32
* Raspberry Pi 4
* Sensore BMP280
* Ventola Noctua NF-A4x10 5V PWM
* Display OLED
* Modulo RTC (Real-Time Clock)
***

## Descrizione del progetto <a name="introduzione"></a>
![alt text](https://github.com/flaviagioiello/SOD-gruppo3/blob/main/schema.png)

Tramite il sensore BMP280 vengono monitorati costantemente i parametri
ambientali di temperatura e pressione.
Quando la temperatura misurata supera una soglia predefinita, viene
attivata la ventola regolando la sua velocità in proporzione alla temperatura rilevata
così da mantenere il sistema all’interno di un range di funzionamento accettabile.
I dati rilevati, tra cui temperatura, pressione e velocità della ventola, vengono
visualizzati in tempo reale su un display OLED.
Questi stessi dati, insieme ai relativi dati temporali, vengono trasmessi utilizzando il
protocollo MQTT dalla scheda ESP32 alla Raspberry Pi, che funge da broker MQTT.
La Raspberry Pi ha il compito di archiviare i dati e di agire come un server IoT
MQTT, consentendo ad altri dispositivi connessi alla stessa rete di visualizzare e
accedere ai dati memorizzati e pubblicando tutte le informazioni su una pagina web.
Il modulo RTC è utilizzato per garantire che tutti i dispositivi all’interno del sistema
siano sincronizzati in termini di tempo.
Il modulo RTC potrebbe gradualmente accumulare una piccola deriva nel tempo, il
che può portare a ritardi nel mantenere l’ora esatta. Per affrontare questo problema,
è stato implementato uno script eseguito sulla Raspberry Pi che estrae l’orario da
un server NTP (Network Time Protocol) e successivamente lo trasmette all’RTC
attraverso il protocollo MQTT.
Il tutto è gestito da Freertos in modo da eseguire molteplici task concorrenti con
priorità diverse e garantire che le attività abbiano sempre l’accesso alle risorse
necessarie.
## Configurazione e setup <a name="conf"></a>

### Configurazione Hardware <a name="hardware"></a>
![alt text](https://github.com/flaviagioiello/SOD-gruppo3/blob/main/setup.jfif)

#### Sensore BMP280 <a name="bmp280"></a>
* Collegare il pin VCC (cavo arancione) del BMP280 al pin 3.3V dell’ESP32 per
l’alimentazione.
* Collegare il pin GND (cavo bianco) del BMP280 al pin GND dell’ESP32 per il
collegamento a terra.
* Collegare Il pin SDI (cavo arancione) del BMP280 al pin SDA (GPIO21 su
ESP32) dell’ESP32 per la comunicazione I2C.
* Collegare Il pin SCK (cavo blu) del BMP280 al pin SCL (GPIO22 su ESP32)
dell’ESP32 per la comunicazione I2C.

#### Display OLED <a name="oled"></a> 
* Collegare il cavo VIN (cavo rosso) del display OLED a una tensione appropriata
(solitamente 3,3 V per ESP32).
* Collegare il cavo GND (cavo nero) del display OLED al GND della ESP32.
* Collegare il cavo SDA (cavo blu) del display OLED al pin SDA (GPIO21) della
ESP32.
* Collegare il pin SCL (cavo giallo) del display OLED al pin SCL (GPIO22) della
ESP32.

#### Modulo RTC <a name="rtc"></a>
* Collegare il pin VCC (cavo rosso) del modulo RTC al pin 3.3V dell’ESP32 per
l’alimentazione.
* Collegare il pin GND (cavo nero) del modulo RTC al pin GND dell’ESP32 per
stabilire il collegamento a terra.
* Collegare il pin SDA (cavo giallo) del modulo RTC al pin GPIO21 dell’ESP32
per la comunicazione dati I2C.
* Collegare il pin SCL (cavo verde) del modulo RTC al pin GPIO22 dell’ESP32
per la comunicazione clock I2C

#### Ventola NOCTUA NF-A4x10 5V PWM <a name="ventola"></a>
* Collegare il pin GND della ventola (cavo grigio) ad uno dei pin GND della
scheda per il collegamento a terra.
* Collegare il pin VCC della ventola (cavo rosso) al pin 5V della scheda affichè
venga alimentata correttamente.
* Collegare il pin TACHO della ventola (cavo verde) al pin GPIO16 della scheda.
Questo collegamento è fondamentale per l’acquisizione dei dati dal tachimetro,
necessaria per il calcolo della velocità della ventola.
* Collegare l’ultimo pin PWM della ventola (cavo giallo) al pin GPIO17 della
scheda. Questo pin serve per acquisire l’intensità del segnale PWM utilizzato
per alimentare la ventola.

### Setup Software <a name="software"></a>

#### Setup Raspberry Pi 4 <a name="raspberry"></a>
* Scaricare e installare [Raspberry Pi Imager](https://www.raspberrypi.org/software/).
* Avviare Raspberry Pi Imager.
* Selezionare l’immagine del sistema operativo Raspberry PI OS (32 bit).
* Selezionare la scheda SD.
* Scrivere l’immagine.
* Una volta completato il processo di scrittura, rimuovere la scheda SD in modo sicuro dal computer e inserirla
nella Raspberry Pi. Collegare tutte le periferiche necessarie e alimentare la Raspberry Pi.
* Configurare il sistema operativo seguendo le istruzioni sullo schermo.
* Aggiornare il sistema con il comando:
```bash
sudo apt update
sudo apt upgrade
```
* Installare le applicazioni desiderate.

#### Setup ESP32 <a name="esp32"></a>
* Installazione di [Arduino IDE](https://www.arduino.cc/en/software).
* Aprire Arduino IDE.
* Andare su *File > Preferenze* e nella finestra delle preferenze, inserire l’URL *https://dl.espressif.com/dl/package_esp32_index.json* nella sezione *URL
aggiuntive per il Gestore schede*.
* Andare su *Strumenti > Gestore schede*, cercare *esp32* e installare il supporto per ESP32.
* Collegare l’ESP32 al Computer tramite il cavo USB.
* Nell’IDE, aprire un nuovo progetto.
* Selezionare il modello ESP32 Dev Module dal menu *Strumenti > Scheda*.
* Selezionare la porta COM corretta a cui è collegato l’ESP32 dal menu "*trumenti > Porta*.
* Caricare il programma di esempio *WiFi > WiFiScan* sull’ESP32 per verificare che tutto funzioni correttamente.

#### Installazione delle librerie <a name="librerie"></a>
* Aprire Arduino IDE.
* Seleziona *Gestisci librerie*.
* Cercare *nome_libreria* per installarla. Le librerie da installare sono:
  * Wire
  * Adafruit_BMP280
  * Adafruit_SSD1306
  * Adafruit GFX Library
  * RTClib
  * WiFi
  * Arduino_JSON
  * FreeRTOS

## Guida al codice <a name="guida"></a>

***
## Software utilizzati <a name="software"></a>
* [Arduino IDE](https://www.arduino.cc/en/software)
* [Visual Studio Code](https://code.visualstudio.com/download)
* [Raspbian GNU/Linux 11 (bullseye)](https://www.raspberrypi.com/software/operating-systems/)
* [FreeRTOS (Real-Time Operating System)](https://www.freertos.org/a00104.html)
***

## Autori <a name="autori"></a>
<ul type="disc">
 <li><a href="https://github.com/flaviagioiello">Flavia Gioiello</a></li>
 <li><a href="https://github.com/adelioA">Adelio Antonini</a></li>
 <li><a href="https://github.com/fioregarzarella">Fiore Garzarella</a></li>
 <li><a href="https://github.com/Lollocik">Lorenzo Cichella</a></li>
</ul>

