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

// Calcul de la consommation avec les ACS712 (5A ou 20A)
// Thanks to http://forum.arduino.cc/index.php?topic=179541.0
int determineVQ(int PIN) {
  long VQ = 0;
  //read 5000 samples to stabilise value
  for (int i=0; i<5000; i++) {
    VQ += analogRead(PIN);
    delay(1);//depends on sampling (on filter capacitor), can be 1/80000 (80kHz) max.
  }
  VQ /= 5000;
  return int(VQ);
}

int adc_zero = determineVQ(AMP_CUIS_SIG);  //autoadjusted relative digital zero

const unsigned long sampleTime = 100000;   // sample over 100ms, it is an exact number of cycles for both 50Hz and 60Hz mains
const unsigned long numSamples = 500;      // choose the number of samples to divide sampleTime exactly, but low enough for the ADC to keep up
const unsigned long sampleInterval = sampleTime/numSamples;  // the sampling interval, must be longer than then ADC conversion time
float readCurrent(int PIN, int AMP) {
  float COEF;
  unsigned long currentAcc = 0;
  unsigned int count = 0;
  unsigned long prevMicros = micros() - sampleInterval;
  while (count < numSamples) {
    if (micros() - prevMicros >= sampleInterval) {
      int adc_raw = analogRead(PIN) - adc_zero;
      currentAcc += (unsigned long)(adc_raw * adc_raw);
      ++count;
      prevMicros += sampleInterval;
    }
  }
  if (AMP == 20) {
    COEF = 50.000;
  }
  else if (AMP == 5) {
    COEF = 27.0273;
  }
  float rms = sqrt((float)currentAcc/(float)numSamples) * (COEF / 1024.00);
  return rms;
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
  pinMode(RELAIS_CH1, OUTPUT);
  pinMode(RELAIS_CH2, OUTPUT);
  pinMode(RELAIS_CH3, OUTPUT);
  pinMode(RELAIS_CH4, OUTPUT);
  pinMode(AMP_CUIS_SIG, INPUT);
  adc_zero = determineVQ(AMP_CUIS_SIG);
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
          client.print(readCurrent(AMP_CUIS_SIG, 20));
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

}



