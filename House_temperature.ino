

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266TelegramBOT.h>

#include <OneWire.h>

// Inizializzo valori rete-wifi
char ssid[] = "SpeedTouch8350DB";     // Nome della rete SSID WI-FI
char password[] ="valentina";   // Password della rete WI-FI

// Inizializzo valori bot telegram
#define BOTtoken "274081276:AAGv55295Rxu4gK2KIrQ7-Hq1GBOCcnHRj0"  //token of TestBOT
#define BOTname "Chameleon_bot"  //name of Bot
#define BOTusername "@Mychameleon_bot"  //username of Bot
#define ID_TELEGRAM "xxxxxxxxxxxxxxxxxxxxxxx"   // ID Telegram of Bot

// Inizializzo valori gestione temperatura
float temperature;
float old_temperature;
short temp_max_raggiunta=false;
OneWire ds(14);
#define MAX_TEMP_OFF_CALDAIA 23
#define MIN_TEMP_ON_CALDAIA  22


//pin gestione caldaia
#define PIN_GESTIONE_CALDAIA 13


//prototipo di funzioni
float getTemp();
void Bot_EchoMessages();
TelegramBOT bot(BOTtoken, BOTname, BOTusername);



int Bot_mtbs = 1000; //tempo per la scansione tra i messaggi
long Bot_lasttime;   


void setup() {

  
  Serial.begin(115200); //inizializzo seriale
  delay(3000);
  pinMode(PIN_GESTIONE_CALDAIA, OUTPUT);       // Setto il pin come uscita
  digitalWrite(PIN_GESTIONE_CALDAIA, HIGH);   // Imposto stato alto (logica inversa il rele è spento)

  // Connessione alla rete wifi
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, password) != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  //inizializzo bot telegram
  bot.begin();      // launch Bot functionalities

  //acquisisco temperatura e controllo se ho superato o meno la temperatura raggiunta
  temperature = getTemp();
  if(temperature>MAX_TEMP_OFF_CALDAIA)
  {
    temp_max_raggiunta=true;
  }
  else
  {
    temp_max_raggiunta=false;
  }
  old_temperature=temperature;
}

void loop() {


  //gestione scansione messaggi e controllo temperatura con spontanee
  //un'altro metodo è di usare il timer attraverso gli interrupt
  //qui utilizzo millis() che conta il tempo da quando parte il programma
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    bot.getUpdates(bot.message[0][1]);   // launch API GetUpdates up to xxx message
    Bot_EchoMessages();                  // reply to message with Echo
    Bot_lasttime = millis();
    temperature = getTemp();
    if(old_temperature!=temperature)
    {
      Serial.println(temperature);
      if(temperature>MAX_TEMP_OFF_CALDAIA)
      {
        if(temp_max_raggiunta)
        {
           String invia_temp="Temperatura massima raggiunta di: "+ String(temperature)+"°C";
           bot.sendMessage(ID_TELEGRAM,invia_temp, "");    
           bot.sendMessage(ID_TELEGRAM,"Avvio spegnimento caldaia", "");     
           temp_max_raggiunta=false;
        }
      }
      else if(temperature<MIN_TEMP_ON_CALDAIA)
      {
        if(!temp_max_raggiunta)
        {
          temp_max_raggiunta=true;
          String invia_temp="Temperatura minima raggiunta di: "+ String(temperature)+"°C";
          bot.sendMessage(ID_TELEGRAM,invia_temp, ""); 
          bot.sendMessage(ID_TELEGRAM,"Avvio accensione caldaia", "");      
        }
      }
      old_temperature=temperature;
      
    }


  }
}


/********************************************
 * EchoMessages - function to Echo messages *
 ********************************************/

void Bot_EchoMessages() {
  for (int i = 1; i < bot.message[0][0].toInt() + 1; i++)      {
    Serial.println(bot.message[i][5]);

    if (bot.message[i][5] == "on") {
      digitalWrite(PIN_GESTIONE_CALDAIA, LOW);   // ATTIVO RELE CALDAIA
      bot.sendMessage(ID_TELEGRAM, "Led is ON", "");  //id del bot
    }
    else if (bot.message[i][5] == "off") {
      digitalWrite(PIN_GESTIONE_CALDAIA, HIGH);  // DISATTIVO RELE CALDAIA
      bot.sendMessage(ID_TELEGRAM, "Led is OFF", ""); //id del bot
    }
    else if (bot.message[i][5] == "temp") {
      String invia_temp="Temp home is: "+ String(temperature)+"°C";
      bot.sendMessage(ID_TELEGRAM,invia_temp, ""); //id del bot
    }

    else if (bot.message[i][5] == "start") {
      String wellcome = "Benvenuto nel tuo Robot personale";
      String wellcome1 = "on : Attiva caldaia";
      String wellcome2 = "off :  Disattiva caldaia";
      String wellcome3 = "temp :  Visualizza temperatura stanza";
      String wellcome4 = "foto : scatta una foto";
      bot.sendMessage(bot.message[i][4], wellcome, "");
      bot.sendMessage(bot.message[i][4], wellcome1, "");
      bot.sendMessage(bot.message[i][4], wellcome2, "");
      bot.sendMessage(bot.message[i][4], wellcome3, "");
      bot.sendMessage(bot.message[i][4], wellcome4, "");
      }
    else if (bot.message[i][5] == "foto") {
      String invia_foto="Questa funzione è in fase di sviluppo";
      bot.sendMessage(ID_TELEGRAM , invia_foto,""); // id del bot
    }
      
    else
    {
      
      bot.sendMessage(ID_TELEGRAM, "Hai scritto:", "");     
      bot.sendMessage(ID_TELEGRAM, bot.message[i][5], "");  //Echo Messaggio di ritorno non gestito
      bot.sendMessage(ID_TELEGRAM, "RITENTA SARAI PIU' FORTUNATO -- MA SAI SCRIVERE","");//
      bot.sendMessage(ID_TELEGRAM, "Mi dispiace le mie risposte sono limitate, Devi farmi le domande giuste",""); 
    }

    
  }
  bot.message[0][0] = "";   // All messages have been replied - reset new messages
}

/********************************************
 * Gestione temperatura
 ********************************************/

 float getTemp(){
  //returns the temperature from one DS18B20 in DEG Celsius
 
  byte data[12];
  byte addr[8];
 
  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
      return -1000;
  }
 
  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return -1000;
  }
 
  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
      return -1000;
  }
 
  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end
 
  byte present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE); // Read Scratchpad
 
  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }
 
  ds.reset_search();
 
  byte MSB = data[1];
  byte LSB = data[0];
 
  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;
 
  return TemperatureSum;
 
}



