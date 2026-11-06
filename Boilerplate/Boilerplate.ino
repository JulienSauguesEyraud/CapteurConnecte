#include <DHT.h>
#include <DHT_U.h>

#include <MPS.h>
#include "config.h"

// ---- Structures de payload ----
struct __attribute__((packed)) CounterPayload {
  uint32_t value;
};

struct __attribute__((packed)) AnalogPayload {
  uint32_t value;
};

struct __attribute__((packed)) DigitalPayload {
  uint8_t value;
};

struct __attribute__((packed)) TemperaturePayload {
  float value;
};

struct __attribute__((packed)) HumidityPayload {
  float value;
};

struct __attribute__((packed)) LedPayload {
  uint8_t state;
  uint8_t color[3];
};

// ---- Variables globales ----
static const uint32_t kSerialBaudRate = 115200;   // débit du port série (bauds)
static const uint32_t kCounterPeriodMs = 2000;    // période du compteur (ms)
static const uint32_t kDataPeriodMs = 1000;    // période des datas (ms)
constexpr uint8_t kAnalogPin = 4;
constexpr uint8_t kDigitalPin = 3;
constexpr uint8_t kDhtPin = 0;
constexpr uint8_t kDhtType = DHT11;
constexpr uint8_t kLedPin = 2;
static CounterPayload counter = {0};
static AnalogPayload analogData = {0};
static DigitalPayload digitalData = {0};
static TemperaturePayload temperatureData = {0};
static HumidityPayload humidityData = {0};
static LedPayload ledData = {0, {255, 255, 255}}; // couleur de base à blanc (255 étant la couleur max et affichant du blanc)
static DHT dht(kDhtPin, kDhtType);

// ---- Subjects ----
constexpr mps::Subject kSubHb = mps::makeSubject("hb_" DEVICE_NAME);
constexpr mps::Subject kSubStatusCounter = mps::makeSubject("status_" DEVICE_NAME "_counter");
constexpr mps::Subject kSubCmdResetCounter = mps::makeSubject("cmd_" DEVICE_NAME "_reset_counter");
constexpr mps::Subject kSubStatusAnalogData = mps::makeSubject("status_" DEVICE_NAME "_analog_data");
constexpr mps::Subject kSubStatusDigitalData = mps::makeSubject("status_" DEVICE_NAME "_digital_data");
constexpr mps::Subject kSubStatusTemperatureData = mps::makeSubject("status_" DEVICE_NAME "_temperature_data");
constexpr mps::Subject kSubStatusHumidityData = mps::makeSubject("status_" DEVICE_NAME "_humidity_data");
constexpr mps::Subject kSubStatusLedData = mps::makeSubject("status_" DEVICE_NAME "_led_data");
constexpr mps::Subject kSubCmdChangeLedState = mps::makeSubject("cmd_" DEVICE_NAME "_change_led_state");
constexpr mps::Subject kSubSetLedColor = mps::makeSubject("set_" DEVICE_NAME "_led_color");


// Fonction permettant de mettre la couleur selon les valeurs du capteur ou à noire si éteinte
void ChangeLedColor() {
  if (ledData.state == 0) 
    rgbLedWrite(kLedPin, 0, 0, 0); // couleur noire pour que la LED soit éteinte (0 étant la couleur min et affichant du noir)
  else
    rgbLedWrite(kLedPin, ledData.color[0], ledData.color[1], ledData.color[2]);
}

// ---- Callbacks ----
// Appelé lors de l'ouverture de la session (callback)
static void onSessionOpened() {
  MPS.publishValue(kSubStatusCounter, counter);
  MPS.publishValue(kSubStatusAnalogData, analogData);
  MPS.publishValue(kSubStatusDigitalData, digitalData);
  MPS.publishValue(kSubStatusTemperatureData, temperatureData);
  MPS.publishValue(kSubStatusHumidityData, humidityData);
  MPS.publishValue(kSubStatusLedData, ledData);
  ChangeLedColor();
}

// Appelé en cas d'échec d'envoi (callback)
static void onSendFailed(uint16_t sequenceNumber, uint32_t subjectId, mps::SendFailureReason, mps::AcknowledgementStatus) {
  Serial.printf("/!\\ envoi %u (subject 0x%08lX) perdu\n", sequenceNumber, (unsigned long)subjectId);
}

// Appelé à la réception de cmd_<DEVICE_NAME>_reset_counter : remet le compteur à 0 et le confirme par status_<DEVICE_NAME>_counter (callback)
static void onCmdReset(const mps::FrameView& message, void*) {
  if (message.payloadLength != 0) return; // ce format n'a pas de payload
  counter.value = 0;
  MPS.publishValue(kSubStatusCounter, counter); // l'IHM voit le résultat réel
}

// Appelé à la réception de cmd_<DEVICE_NAME>_change_led_state : inverse l'état de la LED et le confirme par status_<DEVICE_NAME>_led_data (callback)
static void onCmdChangeLedState(const mps::FrameView& message, void*) {
  if (message.payloadLength != 0) return; // ce format n'a pas de payload
  ledData.state = !ledData.state;
  ChangeLedColor();

  MPS.publishValue(kSubStatusLedData, ledData); // l'IHM voit le résultat réel
}

// Appelé à la réception de set_<DEVICE_NAME>_led_color : mémorise la couleur, l'applique si la LED est allumée et la confirme par status_<DEVICE_NAME>_led_data (callback)
static void onSetLedColor(const mps::FrameView& message, void*) {
  if (message.payloadLength != 3) return;

  ledData.color[0] = message.payload[0];
  ledData.color[1] = message.payload[1];
  ledData.color[2] = message.payload[2];

  ChangeLedColor();

  MPS.publishValue(kSubStatusLedData, ledData);
}

// ---- Fonctions appelées par setup/loop ----
// envoi de heartbeats réguliers sans bloquer (comparaison au temps écoulé sans delay())
void publishHeartbeat(){
  static uint32_t lastHeartbeatTimeMs = 0;
  static const uint16_t kHbPeriod = 1000;
  if(millis() - lastHeartbeatTimeMs >= kHbPeriod) {
    lastHeartbeatTimeMs = millis();
    
    // L'envoi de heartbeats depuis loop() assure au récepteur (IHM) que le code "métier" n'est pas planté
    MPS.publishHeartbeat(kSubHb);
  }
}

// ---- Fonctions Arduino ----
// Exécutée une fois au démarrage
void setup() {
  Serial.begin(kSerialBaudRate); // Démarrage de la communication série à kSerialBaudRate bauds
  MPS.setLogOutput(&Serial); // Définition de la sortie des messages de log de la lib MPS

  dht.begin();

  pinMode(kDigitalPin, INPUT_PULLUP);

  // enregistrement des callbacks
  MPS.setRegisteredCallback(onSessionOpened);
  MPS.setSendFailedCallback(onSendFailed);

  // constante d'adresse vide pour vérifier que l'adresse a été définie dans config.h
  static const uint8_t kUnsetMacAddress[] = {0, 0, 0, 0, 0, 0};
  if(memcmp(kGatewayMacAddress, kUnsetMacAddress, sizeof(kUnsetMacAddress)) == 0) {
    Serial.println("MPS: renseignez kGatewayMacAddress dans config.h (adresse affichee par l'IHM)");
    return;
  }

  MPS.on(kSubCmdResetCounter, onCmdReset);
  MPS.on(kSubCmdChangeLedState, onCmdChangeLedState);
  MPS.on(kSubSetLedColor, onSetLedColor);
  // démarrage du client MPS
  if(!MPS.begin(kGatewayMacAddress, DEVICE_NAME, kWifiChannel)) Serial.println("MPS: demarrage impossible");
}

// Exécutée en boucle (= while(true))
void loop() {
  // appel de la boucle MPS 
  MPS.loop();

  // Appel non bloquant de l'envoi d'HB (polling)
  publishHeartbeat();
  
  static uint32_t lastCounterTimeMs = 0;
  if (millis() - lastCounterTimeMs >= kCounterPeriodMs) {
    lastCounterTimeMs = millis();
    counter.value++;
    MPS.publishValue(kSubStatusCounter, counter);
  }
  static uint32_t lastAnalogDataTimeMs = 0;
  if (millis() - lastAnalogDataTimeMs >= kDataPeriodMs) {
    lastAnalogDataTimeMs = millis();

    analogData.value = analogRead(kAnalogPin);
    MPS.publishValue(kSubStatusAnalogData, analogData);

    digitalData.value = digitalRead(kDigitalPin);
    MPS.publishValue(kSubStatusDigitalData, digitalData);

    temperatureData.value = dht.readTemperature();
    MPS.publishValue(kSubStatusTemperatureData, temperatureData);

    humidityData.value = dht.readHumidity();
    MPS.publishValue(kSubStatusHumidityData, humidityData);

    MPS.publishValue(kSubStatusLedData, ledData);
  }
}
