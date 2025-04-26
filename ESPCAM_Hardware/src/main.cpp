#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <HTTPClient.h>

// WiFi credentials
#define WIFI_SSID "XD iPhone"
#define WIFI_PASSWORD "12341234xdmee"

const char* DATABASE_URL = getenv("DATABASE_URL");
const char* API_KEY = getenv("API_KEY");
const char* USER_EMAIL = getenv("USER_EMAIL");
const char* USER_PASSWORD = getenv("USER_PASSWORD");

// Photo upload endpoint
#define UPLOAD_ENDPOINT "https://nutrients-ai-xd.vercel.app/api/analyze-food"

// Define Firebase and network objects
DefaultNetwork network;
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client, getNetwork(network));
RealtimeDatabase Database;
AsyncResult aResult_no_callback;

// Weight sensor configuration
#define POTENTIOMETER_PIN 35 // Analog pin for potentiometer
#define MAX_WEIGHT 500.0 // Maximum weight in grams
#define MIN_WEIGHT 0.0 // Minimum weight in grams
#define ADC_MAX 4095 // Maximum ADC value for ESP32 (12-bit ADC)
#define ADC_MIN 0 // Minimum ADC value
#define WEIGHT_CHANGE_THRESHOLD 5.0 // Weight change to trigger photo (in grams)

// Camera pins for ESP32-CAM AI-THINKER
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

// Function declarations
void authHandler();
void printResult(AsyncResult &aResult);
void printError(int code, const String &msg);
bool initCamera();
bool captureAndUploadPhoto();

void setup() {
  Serial.begin(115200);
  
  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  
  // Initialize Firebase app
  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  ssl_client.setInsecure();
  initializeApp(aClient, app, getAuth(user_auth), aResult_no_callback);
  authHandler();
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  aClient.setAsyncResult(aResult_no_callback);
  
  // Set up the analog read resolution
  analogReadResolution(12); // ESP32 has 12-bit ADC
  
  // Initialize camera
  if (!initCamera()) {
    Serial.println("Camera initialization failed");
  } else {
    Serial.println("Camera initialization successful");
  }
  
  Serial.println("Setup completed");
}

void loop() {
  authHandler();
  Database.loop();
  
  // Read potentiometer value and calculate weight
  int potValue = analogRead(POTENTIOMETER_PIN);
  float weight = map(potValue, ADC_MIN, ADC_MAX, MIN_WEIGHT * 100, MAX_WEIGHT * 100) / 100.0;
  weight = constrain(weight, MIN_WEIGHT, MAX_WEIGHT);
  
  // Update Firebase with new weight value
  static float lastWeight = -1;
  if (abs(weight - lastWeight) >= 0.5) { // Only update if weight has changed significantly
    lastWeight = weight;
    
    // Update weight in Firebase Realtime Database
    bool status = Database.set<float>(aClient, "/scale/weight", weight);
    if (status) {
      Serial.print("Weight updated: ");
      Serial.print(weight);
      Serial.println(" grams");
      
      // Check if weight change is significant enough to trigger photo capture
      static float lastPhotoWeight = weight;
      if (abs(weight - lastPhotoWeight) >= WEIGHT_CHANGE_THRESHOLD) {
        Serial.println("Significant weight change detected. Taking photo...");
        if (captureAndUploadPhoto()) {
          lastPhotoWeight = weight;
          Serial.println("Photo taken and uploaded successfully");
        } else {
          Serial.println("Failed to capture or upload photo");
        }
      }
    } else {
      printError(aClient.lastError().code(), aClient.lastError().message());
    }
  }
  
  delay(500);
}

// Initialize the ESP32 camera
bool initCamera() {
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
  
  // Initialize with higher resolution for better quality images
  config.frame_size = FRAMESIZE_SVGA; // 800x600
  config.jpeg_quality = 12; // 0-63 lower number means higher quality
  config.fb_count = 1;
  
  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }
  return true;
}

// Capture photo and upload to server
bool captureAndUploadPhoto() {
  // Take picture
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }
  
  // Setup HTTP client for uploading
  HTTPClient http;
  
  // Add timestamp to make each request unique
  String url = String(UPLOAD_ENDPOINT);
  http.begin(url);
  http.addHeader("Content-Type", "image/jpeg");
  
  // Send HTTP POST request with image data
  int httpResponseCode = http.POST(fb->buf, fb->len);
  
  // Return the frame buffer back to be reused
  esp_camera_fb_return(fb);
  
  // Check response
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
    http.end();
    return true;
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
    http.end();
    return false;
  }
}

void authHandler() {
  // Blocking authentication handler with timeout
  unsigned long ms = millis();
  while (app.isInitialized() && !app.ready() && millis() - ms < 120 * 1000) {
    JWT.loop(app.getAuth());
    printResult(aResult_no_callback);
  }
}

void printResult(AsyncResult &aResult) {
  if (aResult.isEvent()) {
    Firebase.printf("Event task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.appEvent().message().c_str(), aResult.appEvent().code());
  }
  if (aResult.isDebug()) {
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(), aResult.debug().c_str());
  }
  if (aResult.isError()) {
    Firebase.printf("Error task: %s, msg: %s, code: %d\n", aResult.uid().c_str(), aResult.error().message().c_str(), aResult.error().code());
  }
}

void printError(int code, const String &msg) {
  Firebase.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
}