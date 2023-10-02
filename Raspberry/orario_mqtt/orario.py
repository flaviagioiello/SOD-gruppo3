# Legge l'orario da un server NTP ogni 24h e lo invia alla ESP32
from datetime import datetime
import paho.mqtt.client as mqtt
import ntplib
import time

# Configurazione MQTT
mqtt_broker = "192.168.178.188"
mqtt_port = 1883
mqtt_topic = "orario"

# Funzione di callback per quando il client MQTT si connette
def on_connect(client, userdata, flags, rc):
    print("Connesso al broker MQTT")

# Inizializza il client MQTT
client = mqtt.Client()
client.on_connect = on_connect

# Connetti il client MQTT al broker
client.connect(mqtt_broker, mqtt_port, 60)
client.loop_start()

# Inizializza il client NTP
ntp_client = ntplib.NTPClient()

while True:
    try:
        # Ottieni l'orario attuale dal server NTP
        response = ntp_client.request('0.pool.ntp.org', version=3)
        timestamp = response.tx_time
        print("NTP Server response:", response)
        print("Timestamp:", response.tx_time)

        # Formatta l'orario in HH:MM:SS
        formatted_time = datetime.fromtimestamp(timestamp).strftime('%H:%M:%S')
        print("Formatted Time:", formatted_time)

        # Invia l'orario alla ESP32 tramite MQTT
        client.publish(mqtt_topic, formatted_time)
        
    except ntplib.NTPException as e:
        print("NTP Exception:", e)
    except Exception as e:
        print("An error occurred:", e)
    
    # Attendi 24 ore prima di inviare nuovamente l'orario
    time.sleep(24 * 60 * 60)

