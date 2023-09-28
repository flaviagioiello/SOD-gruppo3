# Sistemi Operativi Dedicati - Sistema di monitoraggio dei parametri ambientali (Progetto 3)
Questo progetto si basa sull’implementazione di un sistema IoT completo che
combina componenti hardware e software per acquisire, elaborare e condividere dati
ambientali in tempo reale, in grado di rispondere alle loro variazioni in tempo reale.

# Indice
1. [Materiali utilizzati](#materiali)
2. [Descrizione del progetto](#introduzione)
3. [Configurazione e setup](#conf)
    1. [p](#p)
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
Il principale compito del sistema è quello di monitorare costantemente i parametri
ambientali di temperatura e pressione tramite il sensore BMP280.
In particolare, quando la temperatura misurata supera una soglia predefinita, viene
attivata la ventola regolando la sua velocità in proporzione alla temperatura rilevata
così da mantenere il sistema all’interno di un range di funzionamento accettabile.
I dati rilevati, tra cui temperatura, pressione e velocità della ventola, vengono
visualizzati in tempo reale su un display OLED.
Questi stessi dati, insieme ai relativi timestamp, vengono trasmessi utilizzando il
protocollo MQTT dalla scheda ESP32 alla Raspberry Pi, che funge da broker MQTT.
La Raspberry Pi ha il compito di archiviare i dati e di agire come un server IoT
MQTT, consentendo ad altri dispositivi connessi alla stessa rete di visualizzare e
accedere ai dati memorizzati.
Il modulo RTC è utilizzato per garantire che tutti i dispositivi all’interno del sistema
siano sincronizzati in termini di tempo.
Il tutto è gestito da Freertos in modo da eseguire molteplici task concorrenti con
priorità diverse e garantire che le attività abbiano sempre l’accesso alle risorse
necessarie
## Configurazione e setup <a name="conf"></a>

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

