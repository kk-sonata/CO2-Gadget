// clang-format off
/*****************************************************************************************************/
/*********                                                                                   *********/
/*********                            SETUP MQTT FUNCTIONALITY                               *********/
/*********                                                                                   *********/
/*****************************************************************************************************/
// clang-format on

// MQTT
#include <PubSubClient.h>
char MQTT_message[256];
PubSubClient MQTT_client(espClient);
char received_payload[384];
String MQTT_send_topic;
String MQTT_receive_topic;

//JSON
#include <ArduinoJson.h>
StaticJsonDocument<384> jsonBuffer;

// device id, automatically filled by concatenating the last three fields of the wifi mac address, removing the ":" in betweeen, in HEX format. Example: ChipId (HEX) = 85e646, ChipId (DEC) = 8775238, macaddress = E0:98:06:85:E6:46
String anaire_device_id;

// Init to default values; if they have been chaged they will be readed later, on initialization
struct MyConfigStruct {
  char anaire_device_name[24];                                // Device name; default to anaire_device_id
  // uint16_t CO2ppm_warning_threshold = 700;                    // Warning threshold; default to 700ppm
  // uint16_t CO2ppm_alarm_threshold = 1000;                     // Alarm threshold; default to 1000ppm
  char MQTT_server[24] = "mqtt.anaire.org";                   // MQTT server url or public IP address. Default to Anaire Portal on portal.anaire.org
  uint16_t MQTT_port = 80;                                    // MQTT port; Default to Anaire Port on 30183
  // boolean acoustic_alarm = true;                              // Global flag to control acoustic alarm; default to true
  // boolean self_calibration = false;                           // Automatic Baseline Correction of CO2 sensor; default to false
  // uint16_t forced_recalibration_reference = 420;              // Forced Recalibration value; default to 420ppm
  // uint16_t temperature_offset = 600;                          // temperature offset for SCD30 CO2 measurements: 600 by default, because of the housing
  // uint16_t altitude_compensation = 600;                       // altitude compensation for SCD30 CO2 measurements: 600, Madrid altitude
  // char wifi_user[24];                                         // WiFi user to be used on WPA Enterprise. Default to null (not used)
  // char wifi_password[24];                                     // WiFi password to be used on WPA Enterprise. Default to null (not used)
} AnaireConfig;

// Measurements
int CO2ppm_accumulated = 0;   // Accumulates co2 measurements for a MQTT period
int CO2ppm_samples = 0;       // Counts de number of samples for a MQTT period

// device status
boolean err_global = false;
boolean err_wifi = false;
boolean err_MQTT = false;
boolean err_sensor = false;

// Measurements loop: time between measurements
unsigned int measurements_loop_duration = 10000;  // 10 seconds
unsigned long measurements_loop_start;            // holds a timestamp for each control loop start

// MQTT loop: time between MQTT measurements sent to the cloud
unsigned int MQTT_loop_duration = 60000;          // 60 seconds
unsigned long MQTT_loop_start;                    // holds a timestamp for each cloud loop start
unsigned long lastReconnectAttempt = 0;           // MQTT reconnections

// Errors loop: time between error condition recovery
unsigned int errors_loop_duration = 60000;        // 60 seconds
unsigned long errors_loop_start;                  // holds a timestamp for each error loop start

void MQTT_Reconnect() { // MQTT reconnect function
  // Try to reconnect only if it has been more than 5 sec since last attemp
  unsigned long now = millis();
  if (now - lastReconnectAttempt > 5000) {
    lastReconnectAttempt = now;
    Serial.print("Attempting Anaire Cloud MQTT connection...");
    // Attempt to connect
    if (MQTT_client.connect(anaire_device_id.c_str())) {
      err_MQTT = false;
      Serial.println("Anaire Cloud MQTT connected");
      lastReconnectAttempt = 0;
      // Once connected resubscribe
      MQTT_client.subscribe(MQTT_receive_topic.c_str());
      Serial.print("Anaire Cloud MQTT connected - Receive topic: ");
      Serial.println(MQTT_receive_topic);
    } else {
      err_MQTT = true;
      Serial.print("failed, rc=");
      Serial.print(MQTT_client.state());
      Serial.println(" try again in 5 seconds");
    }
  }
}

void Send_Message_Cloud_App_MQTT() { // Send measurements to the cloud
                                     // application by MQTT
  // Print info
  Serial.print("Sending Anaire Cloud MQTT message to the send topic: ");
  Serial.println(MQTT_send_topic);
  sprintf(MQTT_message,
          "{id: %s,CO2: %d,humidity: %f,temperature: %f,VBat: %f}",
          anaire_device_id.c_str(), (int)co2, hum, temp, battery_voltage);
  Serial.print(MQTT_message);
  Serial.println();

  // send message, the Print interface can be used to set the message contents
  MQTT_client.publish(MQTT_send_topic.c_str(), MQTT_message);
}

void Receive_Message_Cloud_App_MQTT(
    char *topic, byte *payload,
    unsigned int length) {      // callback function to receive configuration
                                // messages from the cloud application by MQTT
  memcpy(received_payload, payload, length);
  Serial.print("Anaire Cloud Message arrived: ");
  Serial.println(received_payload);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(jsonBuffer, received_payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  //   // Update name
  //   if ((jsonBuffer["name"]) && (AnaireConfig.anaire_device_name !=
  //   jsonBuffer["name"])) {
  //     strncpy(AnaireConfig.anaire_device_name, jsonBuffer["name"].as<const
  //     char*>(), sizeof(AnaireConfig.anaire_device_name));
  //     AnaireConfig.anaire_device_name[sizeof(AnaireConfig.anaire_device_name)
  //     - 1] = '\0'; Serial.print("Anaire device name: ");
  //     Serial.println(AnaireConfig.anaire_device_name);
  //     write_eeprom = true;
  //   }

  //   // Update warning threshold
  //   if ((jsonBuffer["warning"]) && (AnaireConfig.CO2ppm_warning_threshold !=
  //   (int)jsonBuffer["warning"])) {
  //     AnaireConfig.CO2ppm_warning_threshold = (int)jsonBuffer["warning"];
  //     Evaluate_CO2_Value();
  //     Serial.print("New warning threshold: ");
  //     Serial.println(AnaireConfig.CO2ppm_warning_threshold);
  //     write_eeprom = true;
  //   }

  //   // Update alarm threshold
  //   if ((jsonBuffer["caution"]) && (AnaireConfig.CO2ppm_alarm_threshold !=
  //   (int)jsonBuffer["caution"])) {
  //     AnaireConfig.CO2ppm_alarm_threshold = (int)jsonBuffer["caution"];
  //     Evaluate_CO2_Value();
  //     Serial.print("New alarm threshold: ");
  //     Serial.println(AnaireConfig.CO2ppm_alarm_threshold);
  //     write_eeprom = true;
  //   }

  //   // Update acoustic alarm
  //   if ((jsonBuffer["alarm"]) && ((AnaireConfig.acoustic_alarm) &&
  //   (jsonBuffer["alarm"] == "OFF"))) {
  //     AnaireConfig.acoustic_alarm = false;
  //     Serial.println("Acoustic alarm value: OFF");
  //     write_eeprom = true;
  //   }

  //   if ((jsonBuffer["alarm"]) && ((!AnaireConfig.acoustic_alarm) &&
  //   (jsonBuffer["alarm"] == "ON"))) {
  //     AnaireConfig.acoustic_alarm = true;
  //     Serial.println("Acoustic alarm value: ON");
  //     write_eeprom = true;
  //   }

  //   // Check MQTT server
  //   if ((jsonBuffer["MQTT_server"]) && (AnaireConfig.MQTT_server !=
  //   jsonBuffer["MQTT_server"])) {
  //     strncpy(AnaireConfig.MQTT_server, jsonBuffer["MQTT_server"],
  //     sizeof(AnaireConfig.MQTT_server));
  //     AnaireConfig.MQTT_server[sizeof(AnaireConfig.MQTT_server) - 1] = '\0';
  //     Serial.print("MQTT Server: ");
  //     Serial.println(AnaireConfig.MQTT_server);
  //     write_eeprom = true;

  //     //Attempt to connect to MQTT broker
  //     if (!err_wifi) {
  //       Init_MQTT();
  //     }
  //   }

  //   // Check MQTT port
  //   if ((jsonBuffer["MQTT_port"]) && (AnaireConfig.MQTT_port !=
  //   int(jsonBuffer["MQTT_port"]))) {
  //     AnaireConfig.MQTT_port = int(jsonBuffer["MQTT_port"]);
  //     //strncpy(AnaireConfig.MQTT_port, jsonBuffer["MQTT_port"],
  //     sizeof(AnaireConfig.MQTT_port));
  //     //AnaireConfig.MQTT_port[sizeof(AnaireConfig.MQTT_port) - 1] = '\0';
  //     Serial.print("MQTT Port: ");
  //     Serial.println(AnaireConfig.MQTT_port);
  //     write_eeprom = true;

  //     // Attempt to connect to MQTT broker
  //     if (!err_wifi) {
  //       Init_MQTT();
  //     }
  //   }

  //   // Check FRC value
  //   if ((jsonBuffer["FRC_value"]) &&
  //   (AnaireConfig.forced_recalibration_reference !=
  //   (uint16_t)jsonBuffer["FRC_value"])) {
  //     AnaireConfig.forced_recalibration_reference =
  //     (uint16_t)jsonBuffer["FRC_value"]; write_eeprom = true;
  //   }

  //   // Check temperature offset
  //   if ((jsonBuffer["temperature_offset"]) &&
  //   (AnaireConfig.temperature_offset !=
  //   (uint16_t)jsonBuffer["temperature_offset"])) {
  //     AnaireConfig.temperature_offset =
  //     (uint16_t)jsonBuffer["temperature_offset"]; Set_Temperature_Offset();
  //     write_eeprom = true;
  //   }

  //   // Check altitude_compensation
  //   if ((jsonBuffer["altitude_compensation"]) &&
  //   (AnaireConfig.altitude_compensation !=
  //   (uint16_t)jsonBuffer["altitude_compensation"])) {
  //     AnaireConfig.altitude_compensation =
  //     (uint16_t)jsonBuffer["altitude_compensation"];
  //     Set_Altitude_Compensation();
  //     write_eeprom = true;
  //   }

  //   // If calibration has been enabled, justo do it
  //   if ((jsonBuffer["FRC"]) && (jsonBuffer["FRC"] == "ON")) {
  //     Do_Calibrate_Sensor();
  //     //write_eeprom = true;
  //   }

  //   // Update self calibration
  //   if ((jsonBuffer["ABC"]) && ((AnaireConfig.self_calibration) &&
  //   (jsonBuffer["ABC"] == "OFF"))) {
  //     AnaireConfig.self_calibration = false;
  //     write_eeprom = true;
  //     Set_AutoSelfCalibration();
  //     Serial.println("self_calibration: OFF");
  //     write_eeprom = true;
  //   }

  //   if ((jsonBuffer["ABC"]) && ((!AnaireConfig.self_calibration) &&
  //   (jsonBuffer["ABC"] == "ON"))) {
  //     AnaireConfig.self_calibration = true;
  //     Set_AutoSelfCalibration();
  //     Serial.println("self_calibration: ON");
  //     write_eeprom = true;
  //   }

  //   //print info
  //   Serial.println("Anaire Cloud MQTT update - message processed");
  //   Print_Config();

  //   // If factory reset has been enabled, just do it
  //   if ((jsonBuffer["factory_reset"]) && (jsonBuffer["factory_reset"] ==
  //   "ON")) {
  //     Wipe_EEPROM();   // Wipe EEPROM
  //     ESP.restart();
  //   }

  //   // If reboot, just do it, without cleaning the EEPROM
  //   if ((jsonBuffer["reboot"]) && (jsonBuffer["reboot"] == "ON")) {
  //     ESP.restart();
  //   }

  //   // save the new values if the flag was set
  //   if (write_eeprom) {
  //     Write_EEPROM();
  //   }

  //   // if update flag has been enabled, update to latest bin
  //   // It has to be the last option, to allow to save EEPROM if required
  //   if (((jsonBuffer["update"]) && (jsonBuffer["update"] == "ON"))) {
  //     //boolean result = EEPROM.wipe();
  //     //if (result) {
  //     //  Serial.println("All EEPROM data wiped");
  //     //} else {
  //     //  Serial.println("EEPROM data could not be wiped from flash store");
  //     //}

  //     // Update firmware to latest bin
  //     Serial.println("Update firmware to latest bin");
  //     Firmware_Update();
  //   }
}

void Init_MQTT() { // MQTT Init function
  Serial.print("Attempting to connect to the Anaire Cloud MQTT broker ");
  Serial.print(AnaireConfig.MQTT_server);
  Serial.print(":");
  Serial.println(AnaireConfig.MQTT_port);

  // Attempt to connect to MQTT broker
  MQTT_client.setBufferSize(512); // to receive messages up to 512 bytes length (default is 256)
  // MQTT_client.setServer(AnaireConfig.MQTT_server, AnaireConfig.MQTT_port);
  // MQTT_client.setServer("test.mosquitto.org", 1883);
  MQTT_client.setServer("mqtt.anaire.org", 80);
  MQTT_client.setCallback(Receive_Message_Cloud_App_MQTT);
  MQTT_client.connect(anaire_device_id.c_str());

  if (!MQTT_client.connected()) {
    err_MQTT = true;
    MQTT_Reconnect();
  } else {
    err_MQTT = false;
    lastReconnectAttempt = 0;
    // Once connected resubscribe
    MQTT_client.subscribe(MQTT_receive_topic.c_str());
    Serial.print("Anaire Cloud MQTT connected - Receive topic: ");
    Serial.println(MQTT_receive_topic);
  }
}

void Get_Anaire_DeviceId() { // Get TTGO T-Display info and fill up anaire_device_id with last 6 digits (in HEX) of WiFi mac address
  uint32_t chipId = 0;
  for (int i = 0; i < 17; i = i + 8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
  anaire_device_id = String(chipId, HEX); // HEX format for backwards compatibility to Anaire devices based on NodeMCU board
  Serial.printf("ESP32 Chip model = %s Rev %d.\t", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores and %dMB Flash.\n", ESP.getChipCores(), ESP.getFlashChipSize() / (1024 * 1024));
  Serial.print("Anaire Device ID: ");
  Serial.println(anaire_device_id);
}

void mqttAnaireLoop() {

  // Measurement loop
  if ((millis() - measurements_loop_start) >= measurements_loop_duration) {

    // New timestamp for the loop start time
    measurements_loop_start = millis();

    // Accumulates samples
    CO2ppm_accumulated += co2;
    CO2ppm_samples++;
  }

  // MQTT loop
  if ((millis() - MQTT_loop_start) >= MQTT_loop_duration) {

    // New timestamp for the loop start time
    MQTT_loop_start = millis();

    // Message the MQTT broker in the cloud app to send the measured values
    if ((!err_wifi) && (CO2ppm_samples > 0)) {
      Send_Message_Cloud_App_MQTT();
    }

    // Reset samples after sending them to the MQTT server
    CO2ppm_accumulated = 0;
    CO2ppm_samples = 0;
  }

  // Errors loop
  if ((millis() - errors_loop_start) >= errors_loop_duration) {

    // New timestamp for the loop start time
    errors_loop_start = millis();

    // Try to recover error conditions
    if (err_sensor) {
      Serial.println("--- err_sensor");
      // Setup_Sensor();  // Init co2 sensors
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("--- err_wifi");
      err_wifi = true;
      WiFi.reconnect();
    } else {
      err_wifi = false;
    }

    // Reconnect MQTT if needed
    if ((!MQTT_client.connected()) && (!err_wifi)) {
      Serial.println("--- err_mqtt");
      err_MQTT = true;
    }

    // Reconnect MQTT if needed
    if ((err_MQTT) && (!err_wifi)) {
      Serial.println("--- MQTT reconnect");
      // Attempt to connect to MQTT broker
      MQTT_Reconnect();
      Init_MQTT();
    }
  }
}

void initAnaireCloud() {
    Get_Anaire_DeviceId();
    // Set MQTT topics
    MQTT_send_topic = "measurement"; // Measurements are sent to this topic
    MQTT_receive_topic = "config/" + anaire_device_id; // Config messages will be received in config/id
    if ((WiFi.status() == WL_CONNECTED)) {
      Init_MQTT();
    }
}

// void mqttReconnectAnaire() {
//   static uint64_t timeStamp = 0;
//   uint16_t secondsBetweenRetries = 15; // Keep trying to connect to MQTT broker for 3 minutes (12 times every 15 secs)
//   uint16_t maxConnectionRetries = 12;
//   static uint16_t connectionRetries = 0;
//   if (millis() - timeStamp > (secondsBetweenRetries*1000)) { // Max one try each secondsBetweenRetries*1000 seconds
//     timeStamp = millis();
//     String subscriptionTopic;
//     if (!mqttClientAnaire.connected()) {
//       Serial.print("Attempting MQTT connection to Anaire Cloud...");
//       // Attempt to connect            
//       if (mqttClientAnaire.connect((MQTT_client).c_str())) {
//         Serial.println("connected");
//         Serial.print("rootTopic: ");
//         Serial.println(rootTopic);
//         // Subscribe
//         subscriptionTopic = rootTopic + "/calibration";
//         mqttClientAnaire.subscribe((subscriptionTopic).c_str());
//         printf("Suscribing to: %s\n", subscriptionTopic.c_str());
//         subscriptionTopic = rootTopic + "/ambientpressure";
//         mqttClientAnaire.subscribe((subscriptionTopic).c_str());
//         printf("Suscribing to: %s\n", subscriptionTopic.c_str());
//       } else {
//         ++connectionRetries;
//         mqttClientAnaire.setSocketTimeout(2); //Drop timeout to 2 secs for subsecuents tries
//         Serial.printf(" not possible to connect to %s ", mqttBroker.c_str());
//         Serial.printf("Connection status:  %d. (%d of %d retries)\n", mqttClientAnaire.state(), connectionRetries, maxConnectionRetries); // Possible States: https://pubsubclient.knolleary.net/api#state
//         if (connectionRetries >= maxConnectionRetries) {
//           activeMQTT = false;
//           Serial.println("Max retries to connect to MQTT broker reached, disabling MQTT...");
//         }
//       }
//     }
//   }
// }
