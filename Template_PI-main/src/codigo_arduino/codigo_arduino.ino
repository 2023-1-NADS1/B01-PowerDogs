
#include "WiFi.h" 
#include "esp_camera.h" 
#include "Arduino.h" 
#include "soc/soc.h"           // Disable brownout problems 
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems 
#include "driver/rtc_io.h" 
#include <SPIFFS.h> 
#include <FS.h> 
#include <Firebase_ESP_Client.h> 

//Provide the token generation process info. 

#include <addons/TokenHelper.h> 

#include <Stepper.h> 

  

//Replace with your network credentials 

const char* ssid = "iPhone do Mateus"; 

const char* password = "mateus30"; 

  

// Insert Firebase project API Key 

#define API_KEY "AIzaSyAfKTX7QH2rPWDjmxiQ7gmXvi5JdxwZczo" 

  

// Insert Authorized Email and Corresponding Password 

#define USER_EMAIL "mateusmacedobatista05@gmail.com" 

#define USER_PASSWORD "mateus30" 

  

// Insert Firebase storage bucket ID e.g bucket-name.appspot.com 

#define STORAGE_BUCKET_ID "esp-firebase-1c381.appspot.com" 

  

// Photo File Name to save in SPIFFS 

  

int contador = 0; 

String caminhofoto = "/data/photo"+String(contador)+".jpg"; 

String FILE_PHOTO = caminhofoto; 

  

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER) 

#define PWDN_GPIO_NUM     32 

#define RESET_GPIO_NUM    -1 

#define XCLK_GPIO_NUM      0 

#define SIOD_GPIO_NUM     26 

#define SIOC_GPIO_NUM     27 

#define Y9_GPIO_NUM       35 

#define Y8_GPIO_NUM       34 

#define Y7_GPIO_NUM       39 

#define Y6_GPIO_NUM       36 

#define Y5_GPIO_NUM       21 

#define Y4_GPIO_NUM       19 

#define Y3_GPIO_NUM       18 

#define Y2_GPIO_NUM        5 

#define VSYNC_GPIO_NUM    25 

#define HREF_GPIO_NUM     23 

#define PCLK_GPIO_NUM     22 

  

//ultrassonico 

#define trigger 5 

#define echo 13 

#define trigger2 6 

#define echo2 12 


#include <Stepper.h>

const int stepsPerTurn = 500;

Stepper stepper(stepsPerTurn, 8, 10, 9, 11);

//variaveis ultrassonico 

int ledestoque = 8; 

  

float motor = 7; 

float distancia; 

float duracao; 

float velocidade = 0.0172316; 

float distancia2; 

float duracao2; 

float velocidade2 = 0.0172316; 

  

boolean takeNewPhoto = true; 

  

//Define Firebase Data objects 

FirebaseData fbdo; 

FirebaseAuth auth; 

FirebaseConfig configF; 

  

bool taskCompleted = false; 

  

// Check if photo capture was successful 

bool checkPhoto( fs::FS &fs ) { 

  File f_pic = fs.open( FILE_PHOTO ); 

  unsigned int pic_sz = f_pic.size(); 

  return ( pic_sz > 100 ); 

} 

  

// Capture Photo and Save it to SPIFFS 

void capturePhotoSaveSpiffs( void ) { 

  camera_fb_t * fb = NULL; // pointer 

  bool ok = 0; // Boolean indicating if the picture has been taken correctly 

  do { 

    // Take a photo with the camera 

    Serial.println("Taking a photo..."); 

  

    fb = esp_camera_fb_get(); 

    if (!fb) { 

      Serial.println("Camera capture failed"); 

      return; 

    } 

    // Photo file name 

    Serial.printf("Picture file name: %s\n", FILE_PHOTO); 

    File file = SPIFFS.open(FILE_PHOTO, FILE_WRITE); 

    // Insert the data in the photo file 

    if (!file) { 

      Serial.println("Failed to open file in writing mode"); 

    } 

    else { 

      file.write(fb->buf, fb->len); // payload (image), payload length 

      Serial.print("The picture has been saved in "); 

      Serial.print(FILE_PHOTO); 

      Serial.print(" - Size: "); 

      Serial.print(file.size()); 

      Serial.println(" bytes"); 

    } 

    // Close the file 

    file.close(); 

    esp_camera_fb_return(fb); 

  

    // check if file has been correctly saved in SPIFFS 

    ok = checkPhoto(SPIFFS); 

  } while ( !ok ); 

} 

  

void initWiFi(){ 

  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) { 

    delay(1000); 

    Serial.println("Connecting to WiFi..."); 

  } 

} 

  

void initSPIFFS(){ 

  if (!SPIFFS.begin(true)) { 

    Serial.println("An Error has occurred while mounting SPIFFS"); 

    ESP.restart(); 

  } 

  else { 

    delay(500); 

    Serial.println("SPIFFS mounted successfully"); 

  } 

} 

  

void initCamera(){ 

// OV2640 camera module 

  camera_config_t config; 

  config.ledc_channel = LEDC_CHANNEL_0; 

  config.ledc_timer = LEDC_TIMER_0; 

  config.pin_d0 = Y2_GPIO_NUM; 

  config.pin_d1 = Y3_GPIO_NUM; 

  config.pin_d2 = Y4_GPIO_NUM; 

  config.pin_d3 = Y5_GPIO_NUM; 

  config.pin_d4 = Y6_GPIO_NUM; 

  config.pin_d5 = Y7_GPIO_NUM; 

  config.pin_d6 = Y8_GPIO_NUM; 

  config.pin_d7 = Y9_GPIO_NUM; 

  config.pin_xclk = XCLK_GPIO_NUM; 

  config.pin_pclk = PCLK_GPIO_NUM; 

  config.pin_vsync = VSYNC_GPIO_NUM; 

  config.pin_href = HREF_GPIO_NUM; 

  config.pin_sscb_sda = SIOD_GPIO_NUM; 

  config.pin_sscb_scl = SIOC_GPIO_NUM; 

  config.pin_pwdn = PWDN_GPIO_NUM; 

  config.pin_reset = RESET_GPIO_NUM; 

  config.xclk_freq_hz = 20000000; 

  config.pixel_format = PIXFORMAT_JPEG; 

  

  if (psramFound()) { 

    config.frame_size = FRAMESIZE_UXGA; 

    config.jpeg_quality = 10; 

    config.fb_count = 2; 

  } else { 

    config.frame_size = FRAMESIZE_SVGA; 

    config.jpeg_quality = 12; 

    config.fb_count = 1; 

  } 

  // Camera init 

  esp_err_t err = esp_camera_init(&config); 

  if (err != ESP_OK) { 

    Serial.printf("Camera init failed with error 0x%x", err); 

    ESP.restart(); 

  } 

} 

  

void setup() { 

  // Serial port for debugging purposes 

  Serial.begin(115200); 

  initWiFi(); 

  initSPIFFS(); 

  // Turn-off the 'brownout detector' 

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

  initCamera(); 

  

  //Firebase 

  // Assign the api key 

  configF.api_key = API_KEY; 

  //Assign the user sign in credentials 

  auth.user.email = USER_EMAIL; 

  auth.user.password = USER_PASSWORD; 

  //Assign the callback function for the long running token generation task 

  configF.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h 

  

  Firebase.begin(&configF, &auth); 

  Firebase.reconnectWiFi(true); 

  

  //setup ultrassonico 

  pinMode(motor, OUTPUT); 

  pinMode(trigger, OUTPUT); 

  pinMode(echo, INPUT); 

  pinMode(trigger2, OUTPUT); 

  pinMode(echo2, INPUT); 

  Serial.begin(9600); 

  pinMode(ledestoque, OUTPUT); 


  stepper.setSpeed(60);
  } 

  

void loop() { 

  

  //loop ultrassonico 

  digitalWrite(trigger, 0); 

  delayMicroseconds(5); 

  digitalWrite(trigger, 1); 

  delayMicroseconds(10); 

  digitalWrite(trigger, 0); 

  duracao = pulseIn(echo, 1); 

  distancia = duracao * velocidade; 

  Serial.print("DISTANCIA/PRESENCA - cm: "); 

  Serial.println(distancia); 

  delay(100); 

   

  digitalWrite(trigger2, 0); 

  delayMicroseconds(5); 

  digitalWrite(trigger2, 1); 

  delayMicroseconds(10); 

  digitalWrite(trigger2, 0); 

  duracao2 = pulseIn(echo2, 1); 

  distancia2 = duracao2 * velocidade2; 

  Serial.print("ESTOQUE DE RACAO - cm: "); 

  Serial.println(distancia2); 

  delay(100); 

  if (takeNewPhoto) { 

    contador++; 

    caminhofoto = "/data/photo"+String(contador)+".jpg"; 

     FILE_PHOTO = caminhofoto; 

    capturePhotoSaveSpiffs(); 

    takeNewPhoto = false; 

  } 

  delay(1); 

  if (Firebase.ready() && !taskCompleted){ 

    taskCompleted = true; 

    Serial.print("Uploading picture... "); 

  

    //MIME type should be valid to avoid the download problem. 

    //The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h. 

    if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */, FILE_PHOTO /* path to local file */, mem_storage_type_flash /* memory storage type, mem_storage_type_flash and mem_storage_type_sd */, FILE_PHOTO /* path of remote file stored in the bucket */, "image/jpeg" /* mime type */)){ 

      Serial.printf("\nDownload URL: %s\n", fbdo.downloadURL().c_str()); 

    } 

    else{ 

      Serial.println(fbdo.errorReason()); 

    } 

  } 
   
   if (distancia <100 ){
    takeNewPhoto;
   }

   if (distancia <100)
   {

    for(int i = 0; i <= 10; i++) {
    stepper.step(100);}

    delay (7000);

    for(int i = 0; i <= 10; i++) {
    stepper.step(-100);
      } 
   }
    
   

  if(distancia < 100){ 

    digitalWrite(motor, HIGH); 

       

  

  } else { 

    digitalWrite(motor, LOW);   

  } 

   

  if(distancia2 > 150){ 

    digitalWrite(ledestoque, HIGH); 

    delay(200); 

    digitalWrite(ledestoque, LOW); 

    delay(200); 

  } else { 

    digitalWrite(ledestoque, LOW); 

  }

  for(int i = 0; i <= 10; i++) {
    stepper.step(-100);
      } 

} 
