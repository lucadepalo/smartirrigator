  
/*
  Web client sensore umidita terreno ed elettrovalvola

 Circuit:
 created 11-03-2021
 by Smarteducationlab srl
v2.0 del 23/03/2022
 
 Modded by Luca Depalo
 v3.0 del 09/2023
 */


#include <SPI.h>
#include <WiFiNINA.h>
#include <wdt_samd21.h> //watchdog per resettare a tempo se qualcosa va storto

#include "arduino_secrets.h" //please enter your sensitive data in the Secret tab/arduino_secrets.h

char ssid[] = SECRET_SSID;        // network SSID (name)
char pass[] = SECRET_PASS;    // network password (use for WPA, or use as key for WEP)

char nodePk_AIRR[] = SECRET_NODE_PK_AIRR; //codice unico del dispositivo 
char nodePk_SUT[] = SECRET_SENSOR_PK_SUT; //codice unico del dispositivo 

unsigned long int time_delay = SECRET_TIME_TO_SEND;

char nodeType_AIRR[] = SECRET_NODE_TYPE_AIRR;
char nodeType_SUT[] = SECRET_SENSOR_TYPE_SUT;

char typeSensor[] = SECRET_NODE_TYPE_SENS;
char typeActuator[] = SECRET_NODE_TYPE_ACT;

int valMin = VAL_MIN;
int valMax = VAL_MAX;

char nodeCurrentState = 'C';
int currentPriority = 0;

unsigned long lastConnectionTime_AIRR = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval_AIRR = 60L * 1000L;  //delay between updates, in milliseconds (quindi 1 minuto)

unsigned long lastConnectionTime_SUT = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval_SUT = 60L * 1000L;  //delay between updates, in milliseconds (quindi 1 minuto)
String ricevuto;

int status;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(192,168,87,43);  // numeric IP for Google (no DNS)
char server[] = "personalgarden.000webhostapp.com";
//char server[] = "lucadepalo.dynamic-dns.net";    // name address for Google (using DNS)

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

void setup() 
{//inizio setup
   status = WL_IDLE_STATUS;
  //Initialize serial and wait for port to open:
  Serial.begin(9600);


  /*
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  */
  
  pinMode(8, OUTPUT);
  pinMode(13, OUTPUT);

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  Serial.println("Connected to WiFi");
  printWifiStatus();
  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) 
  {
    Serial.println("connessione server per registrare SENSORE");
    
    // Make a HTTP request:registra sensore
    

    client.println("GET /RegSensor.php?pk="+String(nodePk_SUT)+"&Type="+String(typeSensor)+"&SensorType="+String(nodeType_SUT)+"&ValoreMinimo="+String(valMin)+"&ValoreMassimo="+String(valMax)+" HTTP/1.1");
    client.println("Host: "+String(server));
    client.println("Connection: close");
    client.println();
  }
  delay(1000);
  //PARTE ATTUATORE
  if (client.connect(server, 80)) 
  {
    Serial.println("connessione server per registrare ATTUATORE");
    
     // Make a HTTP request:registra attuatore
    
    client.println("GET /RegAct.php?pk="+String(nodePk_AIRR)+"&Type="+String(typeActuator)+"&ActType="+String(nodeType_AIRR)+" HTTP/1.1");
    client.println("Host: "+String(server));
    client.println("Connection: close");
    client.println();
  }

  //wdt_init ( WDT_CONFIG_PER_16K ); //inizializzo il watchdog per 16 secondi
} //fine SETUP

void loop() {

    //wdt_reset();  

    
  // if ten seconds have passed since your last connection,
  // then connect again and send data:
  

  

  //wdt_reset();

  httpRequest_AIRR();
  
  delay(30000);
 
  httpRequest_SUT();
  
  delay(30000);
}


void httpRequest_AIRR() {
  Serial.println("HTTP REQUEST AIRR");
  // close any connection before send a new request.
  // This will free the socket on the NINA module
  
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:

    client.println("POST /GetActState.php HTTP/1.1");
    client.println(String("Host: ") + server);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    String postData = "fk_nodo_iot=" + String(nodePk_AIRR) + "&priorita=" + String(currentPriority) + "&valore=" + String(nodeCurrentState);
    client.println(postData.length());
    client.println();
    client.println(postData);
    client.println("User-Agent: ArduinoWiFi/1.1");
    

    //wdt_reset();  
    
    // note the time that the connection was made:
    lastConnectionTime_AIRR = millis();
    delay(1000);
    // Read and process the server response:
    while (client.available()) {
      delay(10);
      char c = client.read(); // read what the server is sending
      if (c != '\n' && c != '\r') {
        ricevuto += c; // accumulate in the 'ricevuto' string what you receive from the server
      }
    }

    /*
    // Check if the response is valid:
    if (ricevuto.indexOf("valore:") == -1) {
      Serial.println("Invalid response from server");
      return;
    }
    */

    Serial.println("Server Response: " + ricevuto);

    client.println("Connection: close");
    client.println();

    int index = ricevuto.indexOf("valore:");
    if (index != -1) {
      char valore = ricevuto.charAt(index + 7); // prendi il carattere successivo a "valore:"
      
      if (valore == 'A') {
        Serial.println("\nElettrovalvola APERTA..");
        digitalWrite(8, LOW);  // for relays with inverted logic otherwise HIGH
        nodeCurrentState = 'A';
        digitalWrite(13, HIGH);
      } 
      else if (valore == 'C') {
        Serial.println("\nElettrovalvola CHIUSA..");
        digitalWrite(8, HIGH);  // for relays with inverted logic otherwise LOW
        nodeCurrentState = 'C';
        digitalWrite(13, LOW);
      }
    }
    ricevuto = "";  // Reset the string for the next reading

  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    NVIC_SystemReset(); //reset via SW
  }
}

void httpRequest_SUT() {
  Serial.println("....Entro nel loop \n");
  // close any connection before send a new request.
  // This will free the socket on the NINA module
  client.stop();
 
  
  Serial.println("\n ....MISURA:..................Effettuo una richiesta HTTP \n");
  int somma = 0;
  int numeroMisure = 10; //faccio la media di 10 misure 
  for (int i = 1; i <= numeroMisure; i++) {
    somma = somma + analogRead(A0);
    delay(1000);
  }
  //wdt_reset();  
  int sensorValue = round(somma/numeroMisure); //arrotondo per difetto
  Serial.println();
  Serial.println(String(sensorValue));
  Serial.println("connecting to server: "+(String)server);
  // if there's a successful connection:
  if (client.connect(server, 80)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println("GET /RegData.php?pk="+String(nodePk_SUT)+"&sensorData="+String(sensorValue)+" HTTP/1.1");
    client.println(String("Host: ") + server);
    //client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();
  
    // note the time that the connection was made:
    lastConnectionTime_SUT = millis();
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    //status = WL_IDLE_STATUS;
    NVIC_SystemReset(); //resetta via sw
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
