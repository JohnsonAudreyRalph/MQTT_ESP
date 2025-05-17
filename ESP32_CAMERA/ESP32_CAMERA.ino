#include <WiFi.h>
#include <PubSubClient.h>
#include <esp_camera.h>
#include <base64.h>

// Cấu hình Wi-Fi
const char* ssid = "IoT";         // Điền tên Wi-Fi của bạn
const char* password = "234567Cn"; // Điền mật khẩu Wi-Fi của bạn

// Cấu hình broker MQTT
const char* mqtt_server = "192.168.0.103";  // Địa chỉ broker MQTT (ví dụ: "mqtt.eclipse.org")
const int mqtt_port = 1883;                 // Cổng MQTT
const char* mqtt_topic = "topic/image";     // Topic MQTT để gửi ảnh

WiFiClient espClient;
PubSubClient client(espClient);

// Cấu hình ESP32 Camera
camera_config_t config;

// Kết nối Wi-Fi
void setup_wifi() {
  delay(10);
  // Kết nối đến Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

// Kết nối MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32_Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Chụp ảnh và mã hóa thành base64
String captureImageAndEncode() {
  camera_fb_t *fb = esp_camera_fb_get();  // Lấy ảnh từ camera
  if (!fb) {
    Serial.println("Camera capture failed");
    return "";
  }

  String base64Image = base64::encode(fb->buf, fb->len);  // Mã hóa ảnh thành base64
  esp_camera_fb_return(fb);  // Giải phóng bộ nhớ camera
  return base64Image;
}

// Hàm setup
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Cấu hình camera ESP32-CAM
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 12;
  config.pin_d1 = 13;
  config.pin_d2 = 14;
  config.pin_d3 = 15;
  config.pin_d4 = 16;
  config.pin_d5 = 17;
  config.pin_d6 = 18;
  config.pin_d7 = 19;
  config.pin_xclk = 21;
  config.pin_pclk = 22;
  config.pin_vsync = 23;
  config.pin_href = 25;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;  // Không sử dụng chân reset
  config.xclk_freq_hz = 1000000;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;
  
  esp_camera_init(&config);  // Khởi tạo camera

  // Kết nối MQTT
  reconnect();
}

// Hàm loop
void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  // Chụp ảnh và mã hóa ảnh thành base64
  String imageBase64 = captureImageAndEncode();
  if (imageBase64 != "") {
    Serial.println("Sending image to MQTT...");
    client.publish(mqtt_topic, imageBase64.c_str());  // Gửi ảnh qua MQTT
  }

  delay(5000);  // Gửi ảnh mỗi 5 giây
}
