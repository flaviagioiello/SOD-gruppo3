// Test delle connessioni con i dispositivi via I2C
#include <Wire.h> // Libreria necessaria per la comunicazione I2C
 
 
void setup()
{
  Wire.begin(); // Inizializzazione comunicazione I2C
 
  Serial.begin(115200); // Inizializza la comunicazione seriale a una velocità di 115200 bit al secondo.
  Serial.println("\nI2C Scanner"); // Stampa un messaggio di testo sulla porta seriale per indicare che il programma sta eseguendo la scansione I2C.
}
 
 
void loop()
{
  byte error, address; // Dichiarazione variabili per gestire errori e indirizzi
  int nDevices; // Variabile intera per tenere traccia dei dispositivi I2C trovati
 
  Serial.println("Scanning..."); // Stampa messaggio sulla porta seriale per indicare che la scansione è iniziata
 
  nDevices = 0;
  for(address = 1; address < 127; address++ ) // Ciclo for iterato dall'indirizzo 1 al 126 (0x01 a 0x7F)
  {
    // Lo scanner I2C utilizza il valore restituito da "Wire.end.Transmission"
    // per verificare se un dispositivo ha riconosciuto l'indirizzo
    Wire.beginTransmission(address); // Viene effettuata una trasmissione I2C verso tutti gli indirizzi
    error = Wire.endTransmission();
 
    if (error == 0) // Se error=0 significa che un dispositivo ha risposto all'indirizzo corrente
    {
      Serial.print("I2C device found at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
 
      nDevices++;
    }
    else if (error==4) 
    {
      Serial.print("Unknow error at address 0x");
      if (address<16) 
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
 
  delay(5000);           // aspetta 5 secondi per la prossima scansione
}