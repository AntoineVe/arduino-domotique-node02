// Temperatures
int SONDE_TEMP0 = A0;
int temp_c = 0;
float temp_cm = 0.00;

// Conso cuisinei
int AMP_CUIS_SIG = A1;

// Relais
int RELAIS_CH1 = 2;
int RELAIS_CH2 = 3;
int RELAIS_CH3 = 4;
int RELAIS_CH4 = 5;

// WebServer
#include <UIPEthernet.h>
byte mac[] = { 
  0x00, 0x00, 0x00, 0x01, 0x01, 0x02 };
byte ip[] = { 
  192, 168, 1, 22 };
EthernetServer server(80);
boolean reading = false;
#define BUFSIZ 100

int ACS712(int pin, int amp) {
  int mVperAmp;
  // Cf datasheet pour mVperAmp
  if (amp == 20) {
    mVperAmp = 100;
  } else if (amp == 5) {
    mVperAmp = 185;
  }
  // Recherche les pics // et calcul de l'offset
  long offset = 0;
  int reading = 0;
  int reading_max = 0;
  for(int i = 0; i < 5000; i++) {
    reading = analogRead(pin);
    offset += reading;
    if (reading >= reading_max) {
      reading_max = reading;
    }
    delay(1);
  }
  offset /= 1024;
  // Calcul de la puissance (P=UI)
  // courant alternatif donc utilise la crete
  float max_sensor = (reading_max / 1024.00) * 5000;
  float amps = (max_sensor - offset) / mVperAmp;
  // etrangement, il y a un surplus moyen mesure de 0.365 A
  int watts = round((amps - 0.365) * 230.000);
  return watts;
}

float TMP36(int capteur) {
  unsigned long temperature_raw = 0;
  int sample = 250;
  float temperature_moy = 0.00;
  for(int i = 0; i < sample; i++) {
    temperature_raw = analogRead(capteur);
    temperature_moy += temperature_raw;
    delay(5);
  }
  float temperature = (((((temperature_moy / sample) * 5.00) / 1024.00) * 1000.00) - 500.00) / 10.00;
  return temperature;
}

void setup() {
  Serial.begin(9600);
  pinMode(SONDE_TEMP0, INPUT);
  pinMode(RELAIS_CH1, OUTPUT);
  pinMode(RELAIS_CH2, OUTPUT);
  pinMode(RELAIS_CH3, OUTPUT);
  pinMode(RELAIS_CH4, OUTPUT);
  pinMode(AMP_CUIS_SIG, INPUT);
  Ethernet.begin(mac, ip);
  server.begin();
}

void loop() {
  char clientline[BUFSIZ];
  int index = 0;
  EthernetClient client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          if (index >= BUFSIZ)
            index = BUFSIZ -1;
          continue;
        }
        clientline[index] = 0;
        if (strstr(clientline, "GET /") != 0) {
          char *command;
          command = clientline + 5; 
          (strstr(clientline, " HTTP"))[0] = 0;    
          String comm = String(command[0]);
          comm += String(command[1]);
          Serial.println(comm);
          if (comm == "C1" ) {
            Serial.println("LOW");
            digitalWrite(RELAIS_CH4, LOW);
          }
          if (comm == "C0") {
            digitalWrite(RELAIS_CH4, HIGH);
            Serial.println("HIGH");
          }
        }
        if (c == '\n' && currentLineIsBlank) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/xml");
          client.println();
          client.println("<?xml version=\"1.0\"?>");
          client.println("<root>");
          // Cuisine
          client.println("<node>");
          client.println("\t<name>Cuisine</name>");
          client.println("\t<sensor>");
          client.println("\t\t<name>Temperature</name>");
          client.print("\t\t<value>");
          client.print(TMP36(SONDE_TEMP0));
          client.println("</value>");
          client.println("\t\t<type>TMP36</type>");
          client.println("\t</sensor>");
          client.println("\t<sensor>");
          client.println("\t\t<name>Watts</name>");
          client.print("\t\t<value>");
          client.print(ACS712(AMP_CUIS_SIG, 20));
          client.println("</value>");
          client.println("\t\t<type>ACS712-20</type>");
          client.println("\t</sensor>");
          client.println("</node>");
          client.println("</root>");
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(5);
    client.stop();
  }
  delay(250);
}

