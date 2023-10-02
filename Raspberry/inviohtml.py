# Invia i dati del file JSON alla pagina web
import paho.mqtt.client as mqtt
import requests
import json

# Configurazione del broker MQTT
broker_address = "192.168.178.188"  # Sostituisci con l'indirizzo IP del tuo broker MQTT
broker_port = 1883
topic = "parametri"  # Sostituisci con il tuo topic MQTT

def on_connect(client, userdata, flags, rc):
    print("Connesso al broker MQTT con codice:", rc)
    client.subscribe(topic)

def on_message(client, userdata, msg):
    print("Messaggio ricevuto sul topic", msg.topic)
    print("Contenuto:", msg.payload.decode("utf-8"))
    
    # Invia i dati al server Flask
    dati = json.loads(msg.payload.decode("utf-8"))
    invia_dati_al_server(dati)

def invia_dati_al_server(dati):
    url = "http://192.168.178.188:5000/ricevi-dati"
    headers = {'Content-Type': 'application/json'}
    response = requests.post(url, json=dati, headers=headers)
    print("Risposta dal server:", response.text)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(broker_address, broker_port, 60)
client.loop_forever()

