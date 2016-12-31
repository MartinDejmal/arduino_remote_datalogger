#include <SPI.h>
#include <Ethernet.h>
#include <SFE_BMP180.h>
#include <Wire.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <DHT.h>

// DHT22 - konfigurace
#define DHTPIN 6        // na jakem pinu visi DHT22
#define DHTTYPE DHT22   // budeme pracovat s modulem DHT22  (AM2302)
DHT dht(DHTPIN, DHTTYPE);
//konec konfigurace
 
// OneWire čidlo - DS18B20 - konfigurace
#define ONE_WIRE_BUS 7
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// konec konfigurace
 
// I2C senzory - konfigurace
SFE_BMP180 pressure;
#define ALTITUDE 382
char status;
double T,P,p0,a;
char tlak [6];
char teplota [4];
char vlhkost [4];

// konec konfigurace

// Ethernet shield - konfigurace
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192, 168, 0, 14);
char serverName[] = "192.168.0.250";
int serverPort = 80;
char pageName[] = "/iot/add_value.php";
EthernetClient client;
int totalCount = 0; 
char params[32];
#define delayMillis 60000UL
unsigned long thisMillis = 0;
unsigned long lastMillis = 0;
// konec konfigurace


void setup() {
  Serial.begin(9600);
  Serial.println("<REBOOT>");
j
// I2C - nastaveni
  if (pressure.begin())
    Serial.println("<BMP180_INIT_SUCCESS>");
  else
  {
    Serial.println("<BMP180_INIT_FAIL>");
    while(1);
  }  

// Ethernet - nastaveni
  // disable SD SPI
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);
  Serial.println(F("<ETHERNET_INIT>"));
  Ethernet.begin(mac, ip);

// Onewire
  sensors.begin();

// DHT 
  Serial.println("<DHT_INIT>");  
  dht.begin();

  Serial.println(F("<BOOT_SUCCESS>"));
}

void loop()
{
 
  thisMillis = millis();

  if(thisMillis - lastMillis > delayMillis)
  {
    lastMillis = thisMillis;
//Ethernet POST
    Serial.println("* Ethernet post...");    
    sprintf(params,"i=%i&v=%i",0,totalCount);     
    if(!postPage(serverName,serverPort,pageName,params)) Serial.println(F("* Ethernet post fail..."));
    else Serial.println(F("* Ethernet post pass..."));
    totalCount++;
    Serial.print(F("# Total pass count: "));    
    Serial.println(totalCount,DEC);

//Onewire
    Serial.print("* Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures
    Serial.println("* Request done...");
    Serial.print("# Temperature for device 1 is: ");
    Serial.println(sensors.getTempCByIndex(0)); 

    dtostrf(sensors.getTempCByIndex(0),4,2,teplota);
    sprintf(params,"i=%i&v=%s",25,teplota);     
    Serial.println(params);            
    if(!postPage(serverName,serverPort,pageName,params)) Serial.println(F("* Ethernet post fail..."));
    else Serial.println(F("* Ethernet post pass..."));

//BMP180
    Serial.print("# Provided altitude: ");
    Serial.print(ALTITUDE,1);
    Serial.println(" meters");

    status = pressure.startTemperature();
    if (status != 0)
    {
      delay(status);
      status = pressure.getTemperature(T);
      if (status != 0)
      {
        Serial.print("# Temperature: ");
        Serial.print(T,2);
        Serial.println(" deg C");

        dtostrf(T,4,2,teplota);
        sprintf(params,"i=%i&v=%s",22,teplota);     
        Serial.println(params);            
        if(!postPage(serverName,serverPort,pageName,params)) Serial.println(F("* Ethernet post fail..."));
        else Serial.println(F("* Ethernet post pass..."));
  
        status = pressure.startPressure(3);
        if (status != 0)
        {
          delay(status);
          status = pressure.getPressure(P,T);
          if (status != 0)
          {
            // Print out the measurement:
            Serial.print("# Absolute pressure: ");
            Serial.print(P,2);
            Serial.println(" mb");

            p0 = pressure.sealevel(P,ALTITUDE);

            Serial.print("# Relative (sea-level) pressure: ");
            Serial.print(p0,2);
            Serial.println(" mb");

            dtostrf(p0,6,2,tlak);
            sprintf(params,"i=%i&v=%s",21,tlak);     
            Serial.println(params);            
            if(!postPage(serverName,serverPort,pageName,params)) Serial.println(F("* Ethernet post fail..."));
            else Serial.println(F("* Ethernet post pass..."));
          }
          else Serial.println("error retrieving pressure measurement\n");
        }
        else Serial.println("error starting pressure measurement\n");
      }
      else Serial.println("error retrieving temperature measurement\n");
    }
    else Serial.println("error starting temperature measurement\n");

  //nacteni hodnot z DHT
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  //osetreni chyby vstupu z DHT
  if (isnan(h) || isnan(t)) {
    Serial.println("DHT read fail!");
    return;
  }
  
  Serial.print("# DHT teplota: ");
  Serial.print(t);
  Serial.println(" st. C");
  Serial.print("# DHT vlhkost: ");
  Serial.print(h);
  Serial.println(" %");

  dtostrf(t,4,2,teplota);
  sprintf(params,"i=%i&v=%s",24,teplota);     
  Serial.println(params);            
  if(!postPage(serverName,serverPort,pageName,params)) Serial.println(F("* Ethernet post fail..."));
  else Serial.println(F("* Ethernet post pass..."));

  dtostrf(h,4,2,vlhkost);
  sprintf(params,"i=%i&v=%s",23,vlhkost);     
  Serial.println(params);            
  if(!postPage(serverName,serverPort,pageName,params)) Serial.println(F("* Ethernet post fail..."));
  else Serial.println(F("* Ethernet post pass..."));
  }   
}


byte postPage(char* domainBuffer,int thisPort,char* page,char* thisData)
{
  int inChar;
  char outBuf[64];
  Serial.println(F("* Connecting..."));
  if(client.connect(domainBuffer,thisPort) == 1)
  {
    Serial.println(F("* Connected!"));
    sprintf(outBuf,"POST %s HTTP/1.1",page);
    client.println(outBuf);
    sprintf(outBuf,"Host: %s",domainBuffer);
    client.println(outBuf);
    client.println(F("Connection: close\r\nContent-Type: application/x-www-form-urlencoded"));
    sprintf(outBuf,"Content-Length: %u\r\n",strlen(thisData));
    client.println(outBuf);
    client.print(thisData);
  } 
  else
  {
    Serial.println(F("* Failed!\n"));
    return 0;
  }

  int connectLoop = 0;
  while(client.connected())
  {
    while(client.available())
    {
      inChar = client.read();
      Serial.write(inChar);
      connectLoop = 0;
    }
    delay(1);
    connectLoop++;
    if(connectLoop > 10000)
    {
      Serial.println(F("* Timeout!"));
      client.stop();
    }
  }
  Serial.println(F("* Disconnecting..."));
  client.stop();
  return 1;
}
