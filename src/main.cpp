#include <Arduino.h>
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <SPI.h>
#include <ADXL362.h>
#include "max30105.h"
#include "IIRFilter.h"
#include <Time.h>
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#define WIFI_NETWORK "secret passage_plus"  //the name of the wifi network //at lab it is ESP_Test
#define WIFI_PASSWORD "Afra!17p89#"    //at lab it is esp8266_test
#define WIFI_TIMEOUT_MS 20000
#define WIFI_SSID "secret passage_plus"

// Your Firebase Project Web API Key
#define API_KEY "AIzaSyAIE_5ozQRoAcZaprySgTDVu_YB7QJycik"
// Your Firebase Realtime database URL
#define DATABASE_URL "https://accel-42dfb-default-rtdb.europe-west1.firebasedatabase.app/"

void setup();
void loop();
void measurementsAccel(void *parameter);
void connectToWiFi(void *parameters);
 //this section is for FireBase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

unsigned long t_accel; 
//int Sampling_Time = 2400;  //(sampl_time =40ms-->25 samples/sec--> fs=25Hz
int Sampling_Time_accel = 500; //now I made the accel freq f=2hz or T = 0.5s) //because the HR task takes 5,03s to get a result
//that means that for each second, I take 2 samples (2 Hz = 2 samples/s)
int flag_movement = 0; //that means that there is no movement
//when we detect movement, the variable movement will become 1, then we will turn off the red, IR LED (HR task)

ADXL362 xl;
int16_t XValue, YValue, ZValue, Temperature;
void *a;
static int taskCore = 1;
int distance;
const int num_samples_accel = 9;
int arr_x[num_samples_accel];
int arr_y[num_samples_accel];
int arr_z[num_samples_accel];
double final_x, final_y, final_z, final_temp; 
//int i = 0;
double sumx=0, sumy=0, sumz=0;

void connectToWiFi(void *parameters) {
  for(;;){
    Serial.print("Connecting to Wifi");
    WiFi.mode(WIFI_STA);  //station mode, to connect to an existing wifi
    //AP mode to create a wifi network with esp32, to let someone else configure it by connecting to it 
    WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
    Serial.print("after t has begun");
    unsigned long startAttemptTime = millis();  //uptime of esp32

    while(WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS){
      Serial.print(".");
      delay(100);
    } 

    if (WiFi.status() != WL_CONNECTED){
      Serial.println("Failed!");
      //break;
    } else {
      Serial.print("Connected!");
      Serial.print(WiFi.localIP());
      break;
    }
  }
}

void measurementsAccel(void * pvParameters) {
  for (;;){
    t_accel = millis();
    //t_accel = t_accel/1000; //this way we get less accuracy in the time measurements
    Serial.println();
    sumx = sumy = sumz = 0; //resetting the sums
    flag_movement = 0; //resetting the flag
    for (int i = 0; i < num_samples_accel; i++){
      xl.readXYZTData(XValue, YValue, ZValue, Temperature);
      arr_x[i]=XValue;
      arr_y[i]=YValue;
      arr_z[i]=ZValue;
      sumx += arr_x[i];
      sumy += arr_y[i];
      sumz += arr_z[i];
      //Serial.print("i: "); Serial.print(i); Serial.println();
      if ((i>0) && (arr_x[i] > arr_x[i-1] + 50 || arr_y[i] > arr_y[i-1] + 50 || arr_z[i] > arr_z[i-1] + 50)){ //50 is a threshold I found about movement detection, can be modified
        flag_movement = 1; //a bit like a flag
        //at this point I go to the HR_loop and I turn on the green LED
        break;
      }
      //vTaskDelay(Sampling_Time_accel / portTICK_PERIOD_MS);
      delay(Sampling_Time_accel); //to make sure I take enough samples with the right freq
    }
    //now I am out of the loop
    //Serial.println("temp: "); Serial.print(Temperature); Serial.println();
    final_x = sumx/num_samples_accel;
    final_y = sumy/num_samples_accel;
    final_z = sumz/num_samples_accel;
   
   //distance = sqrt(pow(final_x, 2) + pow(final_y, 2) + pow(final_z,2));
   if (flag_movement == 1){     //do something in the HR loop 
     Serial.println("there was some movement...");
    }else{
      Serial.println("no movement");
    }
    //distance = sqrt(pow(XValue, 2) + pow(YValue, 2) + pow(ZValue,2)); //eucledean distance 
      //Serial.print("XVALUE="); Serial.print(XValue);	 
      //Serial.print("\tYVALUE="); Serial.print(YValue);	 
      //Serial.print("\tZVALUE="); Serial.print(ZValue);	 
      //Serial.print("\tTEMPERATURE="); Serial.println(Temperature);
  /*
      float *ADC_Value_x = (float*)pvPortMalloc( sizeof(float));
      float *ADC_Value_y = (float*)pvPortMalloc( sizeof(float));
      float *ADC_Value_z = (float*)pvPortMalloc( sizeof(float));
      float *ADC_Value_temp = (float*)pvPortMalloc( sizeof(float));
      *ADC_Value_x = float(xl.readXData());
      *ADC_Value_y = float(xl.readYData());
      *ADC_Value_z = float(xl.readYData());
      *ADC_Value_temp = float(xl.readTemp());
      
      
      Serial.print("XVALUE="); Serial.print(*ADC_Value_x);	 
      Serial.print("\tYVALUE="); Serial.print(*ADC_Value_y);	 
      Serial.print("\tZVALUE="); Serial.print(*ADC_Value_z);	 
      Serial.print("\tTEMPERATURE="); Serial.println(*ADC_Value_temp);
      */
      //activity assessment
      



      //String taskMessage = "\nTask running on core ";
      //taskMessage = taskMessage + xPortGetCoreID();
      //Serial.println(taskMessage); 
      //Serial.println(" "); 
  /*    vPortFree(ADC_Value_x);
      vPortFree(ADC_Value_y);
      vPortFree(ADC_Value_z);
      vPortFree(ADC_Value_temp);    */
      
    }
    delay(10);
    //vTaskDelay(Sampling_Time_accel / portTICK_PERIOD_MS); //maybe wont need it, as I am using the delay above 
  }

void setup() {
  xl.begin(5);
  xl.beginMeasure();
  Serial.begin(9600);
  Serial.println("Accelerometer Test "); 
  Serial.println("");
  /* //Firebase section
  connectToWiFi(&a);
  // Assign the api key (required) 
  config.api_key = API_KEY;

  // Assign the RTDB URL (required) 
  config.database_url = DATABASE_URL;

  // Sign up 
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Assign the callback function for the long running token generation task 
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
*/
  xTaskCreatePinnedToCore( 
    measurementsAccel,
    "accel measurements 362",
    10000,
    NULL,
    1,
    NULL,
    taskCore
  );
  Serial.println("Task created...");
}

void loop() {
  delay(10);
  //measurementsAccel();
  //measurementsAccel(&a);
  /* //Firebase section
   if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 4000 || sendDataPrevMillis == 0)){ //it sends a new value to firebase every 15 seconds
    sendDataPrevMillis = millis();
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setInt(&fbdo, "accel/count", count)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    count++;
    
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.setFloat(&fbdo, "accel/XValue accel", XValue)){
    //if (Firebase.RTDB.setFloat(&fbdo, "test/float", 0.01 + random(0,100))){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //now for Y data accel
    if (Firebase.RTDB.setFloat(&fbdo, "accel/YValue accel", YValue)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //now for Z data accel
    if (Firebase.RTDB.setFloat(&fbdo, "accel/ZValue accel", ZValue)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //now for time data accel
    if (Firebase.RTDB.setFloat(&fbdo, "accel/t_accel", t_accel)){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
  */
}

