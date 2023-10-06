# Stabilisce la connessione MQTT e sincronizza lo scambio dei messaggi
# Librerie
import paho.mqtt.client as mqtt
import requests
import json
import subprocess
import time

# Configurazione del broker MQTT
broker_address = "192.168.7.229"  # Sostituisci con l'indirizzo IP del tuo broker MQTT
broker_port = 1883
topic1 = "parametri"  # Sostituisci con il tuo topic MQTT
topic2= "conferma"

# Flag per verificare se è stato ricevuto un messaggio sul topic "conferma"
messaggio_ricevuto = False

# Variabile di timestamp per la logica dei 5 minuti
timestamp_5_minuti = time.time()

def on_connect(client, userdata, flags, rc):
    print("Connesso al broker MQTT con codice:", rc)
    client.subscribe(topic1)
    client.subscribe(topic2)

def on_message(client, userdata, msg):
    print("Messaggio ricevuto sul topic", msg.topic)
    print("Contenuto:", msg.payload.decode("utf-8"))

    if msg.topic == topic1 :
        # Invia i dati al server Flask
        dati = json.loads(msg.payload.decode("utf-8"))
        invia_dati_al_server(dati)
    elif msg.topic == topic2 :
         messaggio_orario(msg.payload.decode("utf-8"))

def invia_dati_al_server(dati):
    url = "http://192.168.7.229:5000/ricevi-dati"
    headers = {'Content-Type': 'application/json'}
    response = requests.post(url, json=dati, headers=headers)
    print("Risposta dal server:", response.text)

def messaggio_orario(payload):
    global messaggio_ricevuto
    print("Messaggio orario ricevuto:", payload)
     # Imposta il flag a True quando riceve un messaggio sul topic "orario"
    messaggio_ricevuto = True


def esegui_script_se_necessario():
    global messaggio_ricevuto
    # Se non è stato ricevuto nessun messaggio sul topic "conferma", esegue lo script
    if not messaggio_ricevuto:
        # Specifica il percorso completo dello script che vuoi eseguire
        script_path = "/home/fiore/Desktop/orario_mqtt/orario.py"
        
        try:
            # Esegue lo script utilizzando subprocess
            subprocess.run(["python", script_path], check=True)
            print("Script eseguito con successo.")
        except subprocess.CalledProcessError as e:
            print("Errore durante l'esecuzione dello script:", e)

# Funzione che imposta messaggio_ricevuto a False ogni 5 minuti
def aggiorna_time():
    global timestamp_5_minuti
    # Aggiorna il timestamp per la prossima verifica dei 5 minuti
    timestamp_5_minuti = time.time()

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(broker_address, broker_port, 60)
client.loop_start()

# Ciclo principale
while True:
      
    # Verifica dei 5 minuti
    if time.time() - timestamp_5_minuti >= 5 * 60:
        aggiorna_time()
        esegui_script_se_necessario()
    # Intervallo di attesa di 1 secondo
    time.sleep(1)

