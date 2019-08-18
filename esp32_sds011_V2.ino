// SDS011 dust sensor sending data to webservice.
// -----------------------------
// By N. Huri.
// sds example - By R. Zschiegner (rz@madavi.de).
// 

#include <SDS011.h>       // for the dispaly
#include <WiFi.h>        // Include the Wi-Fi library
#include <HTTPClient.h> // for the http client
#include <NTPClient.h>  //for the ntp server access - time..
#include <WiFiUdp.h>    //for the ntp server access - time..


//For the OLED Display
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`

SSD1306Wire  display(0x3c, 21, 22);

//change the following to your needs.
String SensorNumber = "Sensor_number";        //sensor number...
const char* ssid     = "XXXXXXXXXXX";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "XXXXXXXXXXX";     // The password of the Wi-Fi network
//==========Below in the http calls there is the server ip and port, please change it as you will.... Now it is 1.1.1.1 and port 81===========
 
float p10, p25; //for the pm values.
int err; // for errors

SDS011 my_sds; // the object to activate the sds.

//We are using the EPS32 for this one with serial port 2.
#ifdef ESP32
HardwareSerial port(2);
#endif

//Var for the restarts
int RestartAfterAfewUpdates = 0; //for the restarts after a few updates.
int RestartIfWifiNotConnected = 0; // for the restart in case wifi cannot connect.
int RestartIfNoTimeUpdate = 0; // for the restart in case wifi cannot connect.
int ledLight = 0; // led light - for more often lead indication.


HTTPClient http10; //for the httpclients to send the data.
HTTPClient http25;   

int httpCode10; //for actually doing the get requests to send the data.
int httpCode25;

int led = 2; //Operational indication LED

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

void setup() {
  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  
  display.clear();
  drawTextOnScreen(SensorNumber, "Init SDS Air");
  display.display(); 
  my_sds.begin(&port); //getting the sds fired up.
  pinMode(led, OUTPUT);
  Serial.begin(115200);
  delay(10);
  Serial.println('\n');
  
  WiFi.begin(ssid, password);             // Connect to the network
  display.clear();
  drawTextOnScreen(SensorNumber, "ConTo Wireless");
  display.display(); 
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println('\n');
  Serial.print(WiFi.status());
  
  while (WiFi.status() != WL_CONNECTED) { //Coonecting to WIFI and Making sure the Wifi Is connected - if not restart after 5 minutes.
    delay(5000);
    Serial.print('.');
    Serial.print(WiFi.status());
    RestartIfWifiNotConnected++;
    if(RestartIfWifiNotConnected == 60){ESP.restart();}
  }
  Serial.println('\n');
  Serial.print(WiFi.status());  
  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  display.clear();
  drawTextOnScreen(SensorNumber, "Wifi Connected");
  display.display(); 
  delay(2000);
  display.clear();
  drawTextOnScreen(SensorNumber, String(WiFi.localIP()));
  display.display(); 
  delay(1000);
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP32 to the computer

// Initialize a NTPClient to get time
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(10800);
}

//Function to display the PM Values.
void drawText(int PM25, int PM10, String timeAndDate) {
    // Text alignment demo
  display.setFont(ArialMT_Plain_16);
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, SensorNumber);
  
  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 17, timeAndDate);

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 31, "PM2.5:  "+String(PM25));

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 45, "PM10:  "+String(PM10));

}

//Function to display the PM Values.
void drawTextOnScreen(String Sensor, String text) {
  display.setFont(ArialMT_Plain_16);
  
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, Sensor);

    display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 17, text);
}

void loop() {
  display.clear();
  drawTextOnScreen(SensorNumber, "Starting Loop");
  display.display(); 
  delay(1000);
Serial.println("Starting Loop.");
Serial.println('\n');
  display.clear();
  drawTextOnScreen(SensorNumber, "Getting time.");
  display.display(); 
  delay(1000);
//Getting the time.
  while(!timeClient.update()) {
    timeClient.forceUpdate();
    delay(500);
    RestartIfNoTimeUpdate++;
    if(RestartIfNoTimeUpdate == 240){ESP.restart();}
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  // Extract time
  timeStamp = formattedDate.substring(splitT+1, formattedDate.length()-4);
  Serial.print("Date and Time: " + dayStamp + " " + timeStamp);
  Serial.println('\n');
  //to send to the update time function.
  String timeAndDate = dayStamp + " " + timeStamp;
  display.clear();
  drawTextOnScreen(SensorNumber, "Waking SDS");
  display.display(); 
  delay(1000);
Serial.println('a');
Serial.println('\n');
  delay(2500);
  my_sds.wakeup(); //waking up the beast.
  delay(2500);
Serial.println('b');
Serial.println('\n');
  display.clear();
  drawTextOnScreen(SensorNumber, "Reading SDS");
  display.display(); 
  delay(1000);
  err = my_sds.read(&p25, &p10); // getting the pm25 and pm10 values. 
Serial.println('c');
Serial.println('\n');
  if (!err) { // making sure there's no errors.
  display.clear();
  drawTextOnScreen(SensorNumber, "Sending to SrvA");
  display.display(); 
  delay(2000);
Serial.println('d');
Serial.println('\n');     
    //http10.begin("http://1.1.1.1:81/WebService.asmx/CreateFile?Filename=Testpm10.txt&Content=" + String(p10)); //preparing to send the data to the webservice.
    //http25.begin("http://1.1.1.1:81/WebService.asmx/CreateFile?Filename=Testpm25.txt&Content=" + String(p25)); //preparing to send the data to the webservice.
    http10.begin("http://1.1.1.1:81/WebService.asmx/CreateFile?Filename=" + SensorNumber + "_pm10.txt&Content=" + String(p10)); //preparing to send the data to the webservice.
    http25.begin("http://1.1.1.1:81/WebService.asmx/CreateFile?Filename=" + SensorNumber + "_pm25.txt&Content=" + String(p25)); //preparing to send the data to the webservice.
  display.clear();
  drawTextOnScreen(SensorNumber, "Sending to SrvB");
  display.display(); 
  delay(2000);
Serial.println('e');
Serial.println('\n'); 
    httpCode10 = http10.GET(); //actually sending the data
    delay(2000);
    httpCode25 = http25.GET();
    delay(2000);
  if ((httpCode10 > 0) && (httpCode25 > 0)) { //Check for the returning code
      display.clear();
      drawTextOnScreen(SensorNumber, "Sending to SrvC");
      display.display(); 
      delay(2000);
      }
  else {
      display.clear();
      drawTextOnScreen(SensorNumber, "Error on HTTP request");
      display.display(); 
    }
Serial.println('f');
Serial.println('\n');     
    http10.end(); //Free the resources
    http25.end(); //Free the resources
  display.clear();
  drawTextOnScreen(SensorNumber, "Sending to SrvD");
  display.display(); 
  delay(2000);
Serial.println('g');
Serial.println('\n');   
    Serial.println("P2.5: " + String(p25));
    Serial.println("P10:  " + String(p10));
    //Displaying the Info
    // clear the display
    display.clear();
    drawText(p25,p10,timeAndDate);
    display.display();
  }
  else //saying there's an error...
  {
  display.clear();
  drawTextOnScreen(SensorNumber, "Didnt Get DATA");
  display.display(); 
  delay(1000);
    Serial.println("didnt get any value from SDS011");
    // clear the display
    display.clear();
    drawText(0,0,"No Value");
    display.display();  
  }

  //Restart esp after 5 updates - for somereason the esp get stuck after 5 times....
  RestartAfterAfewUpdates++;
  if(RestartAfterAfewUpdates == 5){ESP.restart();}
Serial.println('h');
Serial.println('\n');   
  my_sds.sleep(); //getting the beast to sleep.
Serial.println('i');
Serial.println('\n');  
  ledLight = 0; //doind some led work - so see the device is working...
  while (ledLight != 49){
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(3000);               // wait 
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(3000);               // wait 
  ledLight++;
  }
Serial.println('j');
Serial.println('\n');  
  if(WiFi.status() != WL_CONNECTED)// restart the wiki in case it gets disconnected.
  {
  ESP.restart();
  }
}
