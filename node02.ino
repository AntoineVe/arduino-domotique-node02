// Temperatures
int SONDE_TEMP0 = A0;
int temp_c = 0;
float temp_cm = 0.00;

// Conso chauffage
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

// Calcul de la consommation avec les ACS712 (5A ou 20A)
float GetCurrent(int pin, int amp) {
  int reading = 0;
  int reading_max = 0;
  float CurrentSensor = 0.000;
  for(int i = 0; i < 2500; i++) {
    reading = analogRead(pin);
    if (reading >= reading_max) {
      reading_max = reading;
    }
    delay(1);
  }
  float OutputSensorVoltage = (reading_max*5.00)/1023.00;
  if(amp == 20) {
    CurrentSensor = (OutputSensorVoltage - 2.500)/0.100;
  } 
  else {
    CurrentSensor = (OutputSensorVoltage - 2.500)/0.185;
  }
  int Watts = round(CurrentSensor * 230);
  return Watts;
}

int TMP36(int capteur) {
  int temperature = 0;
  float temperature_moy = 0.00;
  for(int i = 0; i < 200; i++) {
    temperature = round((((((analogRead(capteur)) * 5.00) / 1024.00) * 1000) - 500) / 10.00);
    if (temperature != 0 && temperature_moy != 0) {
      temperature_moy = (temperature + temperature_moy) / 2.00;
    }
    if (temperature_moy == 0) {
      temperature_moy = temperature;
    }
    delay(5);
  }
  return round(temperature);
}

void setup() {
  Serial.begin(9600);
  pinMode(SONDE_TEMP0, INPUT);
  pinMode(RELAIS_CH1, INPUT);
  pinMode(RELAIS_CH2, INPUT);
  pinMode(RELAIS_CH3, INPUT);
  pinMode(RELAIS_CH4, INPUT);
  pinMode(AMP_CUIS_SIG, INPUT);
}

void loop() {
  char clientline[BUFSIZ];
  int index = 0;
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= BUFSIZ)
            index = BUFSIZ -1;
          // continue to read more data!
          continue;
        }
        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;
        if (strstr(clientline, "GET /") != 0) {
          // this time no space after the /, so a sub-file!
          char *command;
          command = clientline + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and
          // turn the first character of the substring into a 0 to clear it out.
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
          ;
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/xml");
          client.println();
          client.println("<?xml version=\"1.0\"?>");
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
          client.print(GetCurrent(AMP_CUIS_SIG, 20));
          client.println("</value>");
          client.println("\t\t<type>ACS712-20</type>");
          client.println("\t</sensor>");
          client.println("</node>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }

}


