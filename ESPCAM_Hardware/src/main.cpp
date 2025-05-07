#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <HTTPClient.h>
#include "esp_heap_caps.h"

// WiFi credentials
#define WIFI_SSID "XD"
#define WIFI_PASSWORD "12312345"

const char* DATABASE_URL = getenv("DATABASE_URL");
const char* API_KEY = getenv("API_KEY");
const char* USER_EMAIL = getenv("USER_EMAIL");
const char* USER_PASSWORD = getenv("USER_PASSWORD");

// Photo upload endpoint
#define UPLOAD_ENDPOINT "https://nutrients-ai-xd.vercel.app/api/analyze-food-esp"

// Define Firebase and network objects
DefaultNetwork network;
UserAuth user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp app;
WiFiClientSecure ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client, getNetwork(network));
RealtimeDatabase Database;
AsyncResult aResult_no_callback;

// Button configuration
#define BUTTON_PIN 12      // Digital pin for button input
#define DEBOUNCE_TIME 50   // Debounce time in milliseconds
#define WEIGHT_INCREMENT 50.0 // Weight increment per button press (in grams)
#define CELL_DT 2
#define CELL_SCK 14

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
void printMemoryInfo();

// Button state variables
bool lastButtonState = HIGH;  // Assuming pull-up resistor (button pressed = LOW)
unsigned long lastDebounceTime = 0;
float currentWeight = 0.0;
bool buttonPressed = false;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to start
  
  // Print initial memory status
  printMemoryInfo();
  
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
  
  // Initialize camera first to allocate memory for it
  if (!initCamera()) {
    Serial.println("Camera initialization failed");
  } else {
    Serial.println("Camera initialization successful");
  }
  
  // Print memory status after camera init
  printMemoryInfo();
  
  // Set up button pin with internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Configure client to use less memory for SSL
  ssl_client.setInsecure(); // Use if you don't need certificate verification
  // ssl_client.setBufferSizes(4096, 1024); // Reduce buffer sizes (rx, tx)
  
  // Initialize Firebase app
  Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  initializeApp(aClient, app, getAuth(user_auth), aResult_no_callback);
  
  // Print memory after Firebase init
  printMemoryInfo();
  
  authHandler();
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  aClient.setAsyncResult(aResult_no_callback);
  
  Serial.println("Setup completed");
  
  // Initialize weight in Firebase
  bool status = Database.set<float>(aClient, "/scale/weight", currentWeight);
  if (status) {
    Serial.print("Initial weight set: ");
    Serial.print(currentWeight);
    Serial.println(" grams");
  } else {
    printError(aClient.lastError().code(), aClient.lastError().message());
  }
  
  // Final memory status
  printMemoryInfo();
}

void loop() {
  // Release and acquire resources as needed
  static unsigned long lastFirebaseUpdate = 0;
  const unsigned long firebaseInterval = 5000; // Update Firebase every 5 seconds
  
  // Only perform Firebase operations periodically to save resources
  if (millis() - lastFirebaseUpdate >= firebaseInterval) {
    authHandler();
    Database.loop();
    lastFirebaseUpdate = millis();
  }
  
  // Read button state with debounce
  int reading = digitalRead(BUTTON_PIN);
  
  // Check if button state changed
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // If button state has been stable for debounce period
  if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
    // If button is pressed (LOW with pull-up resistor) and was not pressed before
    if (reading == LOW && !buttonPressed) {
      buttonPressed = true;
      
      // Increment weight
      currentWeight += WEIGHT_INCREMENT;
      
      // Print available memory before Firebase operation
      printMemoryInfo();
      
      // Update weight in Firebase Realtime Database
      bool status = Database.set<float>(aClient, "/scale/weight", currentWeight);
      if (status) {
        Serial.print("Weight updated: ");
        Serial.print(currentWeight);
        Serial.println(" grams");
        
        // Take a photo when weight changes
        Serial.println("Weight changed. Taking photo...");
        if (captureAndUploadPhoto()) {
          Serial.println("Photo taken and uploaded successfully");
        } else {
          Serial.println("Failed to capture or upload photo");
        }
      } else {
        printError(aClient.lastError().code(), aClient.lastError().message());
      }
    } 
    // Button is released
    else if (reading == HIGH && buttonPressed) {
      buttonPressed = false;
    }
  }
  
  // Save the button state for the next loop
  lastButtonState = reading;
  
  delay(10); // Short delay for stability
}

// Print memory information
void printMemoryInfo() {
  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" bytes, Min free heap: ");
  Serial.print(ESP.getMinFreeHeap());
  Serial.print(" bytes, PSRAM free: ");
  Serial.print(ESP.getFreePsram());
  Serial.println(" bytes");
}

// Initialize the ESP32 camera with lower resolution to save memory
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
  
  // Use lower resolution to save memory
  config.frame_size = FRAMESIZE_VGA; // Downgrade from SVGA to VGA (640x480)
  config.jpeg_quality = 15; // 0-63 lower number means higher quality, increasing to save memory
  config.fb_count = 1;
  
  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return false;
  }
  return true;
}


void flushOldFrames() {
  for (int i = 0; i < 3; ++i) { // flush a few times to be sure
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
      esp_camera_fb_return(fb); // release old frame
    }
    delay(100); // small delay to allow camera to refresh
  }
}


bool captureAndUploadPhoto() {
  // Free up any resources before taking photo
  printMemoryInfo();

  flushOldFrames(); // <-- Add this to force a fresh capture

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }
  
  Serial.printf("Picture taken! Size: %zu bytes\n", fb->len);
  
  // Check memory after photo capture
  printMemoryInfo();
  
  bool uploadSuccess = false;
  
  // Setup HTTP client for uploading
  {
    HTTPClient http;
    
    // Add timestamp to make each request unique
    String url = String(UPLOAD_ENDPOINT);
    http.begin(url);
    
    // Set content type header to image/jpeg - this is important!
    http.addHeader("Content-Type", "image/jpeg");
    
    // Send HTTP POST request with image data
    int httpResponseCode = http.POST(fb->buf, fb->len);
    
    // Check response
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
      uploadSuccess = true;
    } else {
      Serial.print("Error on HTTP request: ");
      Serial.println(httpResponseCode);
    }
    
    // Clean up HTTP resources
    http.end();
  }
  
  // Return the frame buffer back to be reused
  esp_camera_fb_return(fb);
  
  // Check memory after upload
  printMemoryInfo();
  
  return uploadSuccess;
}


void authHandler() {
  // Blocking authentication handler with shorter timeout
  unsigned long ms = millis();
  unsigned long timeout = 60 * 1000; // Reduce timeout to 60 seconds
  
  while (app.isInitialized() && !app.ready() && millis() - ms < timeout) {
    JWT.loop(app.getAuth());
    printResult(aResult_no_callback);
    delay(10); // Small delay to prevent watchdog timer issues
  }
  
  if (millis() - ms >= timeout && !app.ready()) {
    Serial.println("Authentication timed out. Continuing without waiting for completion.");
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