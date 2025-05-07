#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"
#include <HTTPClient.h>
#include "esp_heap_caps.h"
#include <HX711.h>              // Add HX711 library
#include <LiquidCrystal_I2C.h>  // Add LCD library
#include <Wire.h>               // Add Wire library for I2C communication

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

// HX711 configuration
#define LOADCELL_DOUT_PIN 2     // HX711 data pin
#define LOADCELL_SCK_PIN  14    // HX711 clock pin
HX711 scale;                    // Create HX711 instance

// LCD configuration (I2C)
#define LCD_ADDRESS 0x27        // I2C address for LCD (default for most I2C LCDs)
#define LCD_COLUMNS 16          // Number of columns on LCD
#define LCD_ROWS    2           // Number of rows on LCD
LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);

// Calibration factor for your specific load cell (you'll need to calibrate)
#define CALIBRATION_FACTOR -215.0  // Example value, replace with your calibrated value

// Weight recording threshold and timing
#define WEIGHT_THRESHOLD 20.0   // Minimum weight change to trigger update (in grams)
#define DEBOUNCE_TIME 1000      // Minimum time between weight updates (in milliseconds)
#define FIREBASE_UPDATE_INTERVAL 5000  // Update Firebase every 5 seconds
#define LCD_SDA 13
#define LCD_SCL 15

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
void setupI2C();
void initLoadCell();
float getWeight();
void updateLCD(float weight);

// Weight tracking variables
float currentWeight = 0.0;
float lastRecordedWeight = 0.0;
unsigned long lastWeightUpdateTime = 0;
unsigned long lastFirebaseUpdateTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial monitor time to start
  
  // Print initial memory status
  printMemoryInfo();
  
  // Set up I2C for LCD (using software I2C on available pins)
  setupI2C();
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  
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
  
  lcd.clear();
  lcd.print("WiFi Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(2000);
  
  // Initialize camera
  lcd.clear();
  lcd.print("Init Camera...");
  if (!initCamera()) {
    Serial.println("Camera initialization failed");
    lcd.setCursor(0, 1);
    lcd.print("Camera Failed!");
  } else {
    Serial.println("Camera initialization successful");
    lcd.setCursor(0, 1);
    lcd.print("Camera Ready!");
  }
  delay(1000);
  
  // Print memory status after camera init
  printMemoryInfo();
  
  // Initialize load cell
  lcd.clear();
  lcd.print("Init Scale...");
  initLoadCell();
  lcd.setCursor(0, 1);
  lcd.print("Scale Ready!");
  delay(1000);
  
  // Configure client to use less memory for SSL
  ssl_client.setInsecure(); // Use if you don't need certificate verification
  
  // Initialize Firebase app
  lcd.clear();
  lcd.print("Init Firebase...");
  Serial.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  initializeApp(aClient, app, getAuth(user_auth), aResult_no_callback);
  
  // Print memory after Firebase init
  printMemoryInfo();
  
  authHandler();
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  aClient.setAsyncResult(aResult_no_callback);
  
  Serial.println("Setup completed");
  lcd.setCursor(0, 1);
  lcd.print("Setup Complete!");
  delay(1000);
  
  // Initialize weight in Firebase
  lcd.clear();
  lcd.print("Ready to weigh");
  lcd.setCursor(0, 1);
  lcd.print("Place item...");
  
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
  // Handle Firebase authentication and database operations periodically
  if (millis() - lastFirebaseUpdateTime >= FIREBASE_UPDATE_INTERVAL) {
    authHandler();
    Database.loop();
    lastFirebaseUpdateTime = millis();
  }
  
  // Get current weight from load cell
  float newWeight = getWeight();
  
  // Update LCD with current weight
  updateLCD(newWeight);
  
  // Check if weight has changed significantly
  if (abs(newWeight - lastRecordedWeight) > WEIGHT_THRESHOLD && 
      millis() - lastWeightUpdateTime > DEBOUNCE_TIME) {
    
    // Update the current weight
    currentWeight = newWeight;
    lastRecordedWeight = newWeight;
    lastWeightUpdateTime = millis();
    
    // Print available memory before Firebase operation
    printMemoryInfo();
    
    // Update weight in Firebase Realtime Database
    bool status = Database.set<float>(aClient, "/scale/weight", currentWeight);
    if (status) {
      Serial.print("Weight updated: ");
      Serial.print(currentWeight);
      Serial.println(" grams");
      
      // Take a photo when weight changes significantly
      Serial.println("Weight changed. Taking photo...");
      lcd.clear();
      lcd.print("Taking photo...");
      
      if (captureAndUploadPhoto()) {
        Serial.println("Photo taken and uploaded successfully");
        lcd.setCursor(0, 1);
        lcd.print("Photo uploaded!");
      } else {
        Serial.println("Failed to capture or upload photo");
        lcd.setCursor(0, 1);
        lcd.print("Photo failed!");
      }
      
      // Return to weight display after a short delay
      delay(1500);
    } else {
      printError(aClient.lastError().code(), aClient.lastError().message());
    }
  }
  
  delay(100); // Short delay for stability
}

// Set up I2C communication for LCD
void setupI2C() {
  // Using GPIO 15 (SCL) and GPIO 13 (SDA) for I2C
  // These pins are typically available on ESP32-CAM
  Wire.begin(LCD_SDA, LCD_SCL);  // SDA, SCL
}

// Initialize HX711 load cell
void initLoadCell() {
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  delay(1000); // Give it time to stabilize
  
  Serial.println("Initializing the scale");
  scale.set_scale(CALIBRATION_FACTOR);  // Set calibration factor
  
  // Tare the scale (set zero point)
  Serial.println("Tare... remove any weights from the scale");
  delay(2000);
  scale.tare();
  Serial.println("Tare complete");
}

// Get weight from load cell
float getWeight() {
  if (scale.is_ready()) {
    float weight = scale.get_units(5);  // Average 5 readings for stability
    if (weight < 0) {
      weight = 0.0;  // Don't allow negative weights
    }
    return weight;
  } else {
    Serial.println("HX711 not found");
    return -1.0;  // Error indicator
  }
}

// Update LCD with weight information
void updateLCD(float weight) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Weight:");
  lcd.setCursor(0, 1);
  
  if (weight < 0) {
    lcd.print("Error reading");
  } else {
    lcd.print(weight, 1);  // Display with 1 decimal place
    lcd.print(" g");
  }
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

// Capture photo and upload to server with memory management
bool captureAndUploadPhoto() {
  // Free up any resources before taking photo
  printMemoryInfo();

  flushOldFrames(); // Force a fresh capture

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