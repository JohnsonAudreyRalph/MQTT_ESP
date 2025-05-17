#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>  // Thư viện JSON

// Cấu hình WiFi
const char* ssid = "DTVT-T5";
const char* password = "Mật_khẩu_WIFI";

// Cấu hình MQTT
const char* mqttServer = "172.16.1.149";
const int mqttPort = 1883;               // Cổng MQTT
const char* mqttTopic = "topic"; // Topic để gửi dữ liệu

WiFiClient espClient;
PubSubClient client(espClient);

// Cấu hình NTPClient
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // GMT+7 (điều chỉnh múi giờ)

// Hàm kết nối WiFi
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // WiFi.begin(ssid, password);
  WiFi.begin(ssid); // Nếu không có mật khẩu

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Hàm callback khi nhận dữ liệu từ MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Nội dung nhận được từ TOPIC: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String message;
  
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  // Phân tích JSON
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.f_str());
    return;
  }

  // Trích xuất dữ liệu từ JSON
  String receivedDate = doc["date"];
  String receivedTime = doc["time"];
  
  // In ra các giá trị nhận được
  Serial.print("Ngày: ");
  Serial.println(receivedDate);
  Serial.print("Giờ: ");
  Serial.println(receivedTime);
}

// Hàm kết nối MQTT
void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(mqttTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Hàm chuyển đổi epoch time thành định dạng ngày dd/mm/yyyy
String formatDate(unsigned long epochTime) {
  int days = epochTime / 86400; // Một ngày có 86400 giây
  int year = 1970;

  // Tính năm
  while (days >= 365) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) { // Năm nhuận
      if (days >= 366) {
        days -= 366;
        year++;
      } else {
        break;
      }
    } else {
      days -= 365;
      year++;
    }
  }

  // Tính tháng
  int monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
    monthDays[1] = 29; // Năm nhuận, tháng 2 có 29 ngày
  }

  int month = 0;
  while (days >= monthDays[month]) {
    days -= monthDays[month];
    month++;
  }
  month++;
  int day = days + 1;

  // Định dạng ngày
  char dateBuffer[11];
  snprintf(dateBuffer, sizeof(dateBuffer), "%02d/%02d/%04d", day, month, year);
  return String(dateBuffer);
}

void setup() {
  Serial.begin(115200);
  setupWiFi();
  
  client.setServer(mqttServer, mqttPort); // Thiết lập MQTT với địa chỉ IP
  client.setCallback(callback);
  
  timeClient.begin(); // Bắt đầu lấy thời gian
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  // Cập nhật thời gian từ NTP
  timeClient.update();
  
  // Lấy epoch time
  unsigned long epochTime = timeClient.getEpochTime();
  
  // Định dạng ngày và giờ
  String formattedDate = formatDate(epochTime);       // Định dạng ngày dd/mm/yyyy
  String time = timeClient.getFormattedTime();        // Định dạng giờ HH:MM:SS
  
  // Gửi dữ liệu qua MQTT mỗi 5 giây
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 5000) {
    lastMsg = millis();
    String message = "{\"date\":\"" + formattedDate + "\",\"time\":\"" + time + "\"}";
    Serial.print("Sending message: ");
    Serial.println(message);
    client.publish(mqttTopic, message.c_str());
  }
}