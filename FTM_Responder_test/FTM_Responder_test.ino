/* Wi-Fi FTM Responder Arduino Example

  This example code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/
#include "WiFi.h"
#include "esp_wifi.h"

// Change the SSID and PASSWORD here if needed
const char *WIFI_FTM_SSID = "WiFi_FTM_Responder";
const char *WIFI_FTM_PASS = "ftm_responder";

void setup() {
  Serial.begin(115200);
  Serial.println("Starting SoftAP with FTM Responder support");

  esp_wifi_set_band_mode(WIFI_BAND_MODE_AUTO);
  // esp_wifi_set_bandwidth( WIFI_IF_AP,WIFI_BW_HT40);

  // Enable AP with FTM support (last argument is 'true')
  WiFi.softAP(WIFI_FTM_SSID, WIFI_FTM_PASS, 1, 0, 4, true);

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_AP,mac);
  char buffer[18];  // 6*2 characters for hex + 5 characters for colons + 1 character for null terminator
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(buffer);
}

void loop() {
  delay(1000);
}
