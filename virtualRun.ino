#include "WiFiEsp.h"

const int sensorPin = 2;
int lastPulseCount = 0;
boolean isRunning = false;
volatile int pulseCount = 0;
volatile unsigned long previousTime = 0; 

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#endif


char ssid[] = "SSID_NAME";            // your network SSID (name)
char pass[] = "PASSWORD";        // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

long debounce = 300;

WiFiEspServer server(80);


void countPulse() {
  // update time from last call 
  unsigned long currentTime = millis();
  unsigned long deltaTime = currentTime - previousTime;

  if(deltaTime<debounce) return;
  pulseCount++; 
  previousTime = currentTime; 
}

void resetCounter() {
  pulseCount=0;
  previousTime=0;
  isRunning=false;
}


void setup()
{
  // initialize serial for debugging
  Serial.begin(115200);
  // initialize serial for ESP module
  Serial1.begin(9600);

  pinMode(sensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(sensorPin), countPulse, RISING);

  // initialize ESP module
  WiFi.init(&Serial1);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  printWifiStatus();
  
  // start the web server on port 80
  server.begin();
}



void loop()
{
  // listen for incoming clients
  WiFiEspClient client = server.available();
  if (client) {
    Serial.println("New client");
    //pulseCount=pulseCount+1;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        
        //parse command
        String f = client.readString();
        int spacePos = f.indexOf(" HTTP");
        String cmd = f.substring(0,spacePos);
        cmd.replace("GET /","");
        Serial.println("--------------- "+cmd);
        // send a standard http response header
        // use \r\n instead of many println statements to speedup data send
        client.print(
        "HTTP/1.1 200 OK\r\n"
        "Content-type:application/json\r\n"
        "Connection: close\r\n"  // the connection will be closed after completion of the response
        "\r\n");

        if (cmd == "run") {
          isRunning = pulseCount != lastPulseCount;
          client.print("{\"s\": \"ok\",\"r\": " + String(isRunning) + ",\"p\":"+String(pulseCount)+"}");
          lastPulseCount = pulseCount;
          break;
        }        
        if (cmd == "reset") {
          client.print("{\"s\": \"ok\"}");
          resetCounter();
          break;
        }
      }
    }
    // give the web browser time to receive the data
    delay(10);

    // close the connection:
    client.stop();
    Serial.println("Client disconnected");
  }
}


void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  
  // print where to go in the browser
  Serial.println();
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  Serial.println();
}
