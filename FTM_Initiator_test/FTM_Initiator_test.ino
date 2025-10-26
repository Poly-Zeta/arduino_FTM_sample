/* Wi-Fi FTM Initiator Arduino Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "WiFi.h"
#include "esp_wifi.h"

#define AP_COUNTS 3

/*
   THIS FEATURE IS SUPPORTED ONLY BY ESP32-S2 AND ESP32-C3
*/

// Change the SSID and PASSWORD here if needed
const char *WIFI_FTM_SSID = "WiFi_FTM_Responder";  // SSID of AP that has FTM Enabled
const char *WIFI_FTM_PASS = "ftm_responder";       // STA Password

// FTM settings
// Number of FTM frames requested in terms of 4 or 8 bursts (allowed values - 0 (No pref), 16, 24, 32, 64)
const uint8_t FTM_FRAME_COUNT = 16;
// Requested time period between consecutive FTM bursts in 100’s of milliseconds (allowed values - 0 (No pref) or 2-255)
const uint16_t FTM_BURST_PERIOD = 4;

// Semaphore to signal when FTM Report has been received
SemaphoreHandle_t ftmSemaphore;
// Status of the received FTM Report
bool ftmSuccess = true;

typedef struct{
  uint8_t mac[6];
  uint8_t channel;
} accessPoint_t;

// FTM report handler with the calculated data from the round trip
// WARNING: This function is called from a separate FreeRTOS task (thread)!
void onFtmReport(arduino_event_t *event) {
  const char *status_str[5] = {"SUCCESS", "UNSUPPORTED", "CONF_REJECTED", "NO_RESPONSE", "FAIL"};
  wifi_event_ftm_report_t *report = &event->event_info.wifi_ftm_report;
  // Set the global report status
  ftmSuccess = report->status == FTM_STATUS_SUCCESS;
  
  char buffer[18];
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", report->peer_mac[0], report->peer_mac[1], report->peer_mac[2], report->peer_mac[3], report->peer_mac[4], report->peer_mac[5]);

  if (ftmSuccess) {
    // The estimated distance in meters may vary depending on some factors (see README file)
    Serial.printf(buffer);
    Serial.printf(",%.2f m, %lu ns, %lu\n", (float)report->dist_est / 100.0, report->rtt_est,report->rtt_raw);
    // Serial.printf("FTM Estimate: Distance: %.2f m, Return Time: %lu ns, raw_rtt:%lu\n", (float)report->dist_est / 100.0, report->rtt_est,report->rtt_raw);
    // Pointer to FTM Report with multiple entries, should be freed after use
    free(report->ftm_report_data);
  } else {
    Serial.print("FTM Error: ");
    Serial.println(status_str[report->status]);

    free(report->ftm_report_data);//add
  }
  // Signal that report is received
  xSemaphoreGive(ftmSemaphore);
}

// Initiate FTM Session and wait for FTM Report
bool getFtmReport() {
  static int counter=0;

  //e.g. 多数のAPのmac，channelを格納しておくとそれらに対して順次FTMを実行
  //36~chはC5のみ対応 それ以外で実行したらどう止まるかは見てない
  accessPoint_t ap_t[AP_COUNTS]={
    {{0xD0, 0xCF, 0x13, 0xE0, 0xB1, 0x55},36},
    {{0x8E, 0xBF, 0xEA, 0x0D, 0xD4, 0x8C}, 1},
    {{0x8E, 0xBF, 0xEA, 0x0D, 0xD6, 0xA4}, 1},
  };

  // const uint8_t target_mac[6] = {0xD0, 0xCF, 0x13, 0xE0, 0xB1, 0x55};

  // if (!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD,36,target_mac)) {
  if (!WiFi.initiateFTM(FTM_FRAME_COUNT, FTM_BURST_PERIOD,ap_t[counter].channel,ap_t[counter].mac)) {
    Serial.println("FTM Error: Initiate Session Failed");
    // return false;
  }

  counter++;
  if(counter>=AP_COUNTS){
    counter=0;
  }

  // Wait for signal that report is received and return true if status was success
  return xSemaphoreTake(ftmSemaphore, portMAX_DELAY) == pdPASS && ftmSuccess;
}

void setup() {
  Serial.begin(115200);

  // Create binary semaphore (initialized taken and can be taken/given from any thread/ISR)
  ftmSemaphore = xSemaphoreCreateBinary();

  // Will call onFtmReport() from another thread with FTM Report events.
  WiFi.onEvent(onFtmReport, ARDUINO_EVENT_WIFI_FTM_REPORT);

  // Connect to AP that has FTM Enabled
  Serial.println("Connecting to FTM Responder");

  esp_wifi_set_band_mode(WIFI_BAND_MODE_AUTO);
  // esp_wifi_set_bandwidth( WIFI_IF_STA,WIFI_BW_HT40);
  WiFi.mode(WIFI_MODE_STA);
  // WiFi.begin(WIFI_FTM_SSID, WIFI_FTM_PASS);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("");
  // Serial.println("WiFi Connected");

  Serial.print("Initiating FTM session with Frame Count ");
  Serial.print(FTM_FRAME_COUNT);
  Serial.print(" and Burst Period ");
  Serial.print(FTM_BURST_PERIOD * 100);
  Serial.println(" ms");

}

void loop() {
  //setup内からloop内にwhileを移したので，コケでも再開するはず
  // Request FTM reports until one fails
  while (getFtmReport());
  delay(1000);
}
