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
  uint32_t value;
};

// ---- Variables globales ----
static CounterPayload counter = {0};
static AnalogPayload analogData = {0};
static DigitalPayload digitalData = {0};
constexpr uint8_t kAnalogPin = 4;
constexpr uint8_t kDigitalPin = 3;
static const uint32_t kSerialBaudRate = 115200;   // débit du port série (bauds)
static const uint32_t kCounterPeriodMs = 2000;    // période du compteur (ms)

// ---- Subjects ----
constexpr mps::Subject kSubHb = mps::makeSubject("hb_" DEVICE_NAME);
constexpr mps::Subject kSubStatusCounter = mps::makeSubject("status_" DEVICE_NAME "_counter");
constexpr mps::Subject kSubCmdResetCounter = mps::makeSubject("cmd_" DEVICE_NAME "_reset_counter");
constexpr mps::Subject kSubStatusAnalogData = mps::makeSubject("status_" DEVICE_NAME "_analog_data");
constexpr mps::Subject kSubStatusDigitalData = mps::makeSubject("status_" DEVICE_NAME "_digital_data");

// ---- Callbacks ----
// Appelé lors de l'ouverture de la session (callback)
static void onSessionOpened() {
  MPS.publishValue(kSubStatusCounter, counter);
  MPS.publishValue(kSubStatusAnalogData, analogData);
  MPS.publishValue(kSubStatusDigitalData, digitalData);
}

// Appelé en cas d'échec d'envoi (callback)
static void onSendFailed(uint16_t sequenceNumber, uint32_t subjectId, mps::SendFailureReason, mps::AcknowledgementStatus) {
  Serial.printf("/!\\ envoi %u (subject 0x%08lX) perdu\n", sequenceNumber, (unsigned long)subjectId);
}

static void onCmdReset(const mps::FrameView& message, void*) {
  if (message.payloadLength != 0) return; // ce format n'a pas de payload
  counter.value = 0;
  MPS.publishValue(kSubStatusCounter, counter); // l'IHM voit le résultat réel
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
  if (millis() - lastAnalogDataTimeMs >= 1000) {
    lastAnalogDataTimeMs = millis();
    analogData.value = analogRead(kAnalogPin);
    MPS.publishValue(kSubStatusAnalogData, analogData);
  }
    static uint32_t lastDigitalDataTimeMs = 0;
  if (millis() - lastDigitalDataTimeMs >= 1000) {
    lastDigitalDataTimeMs = millis();
    digitalData.value = digitalRead(kDigitalPin);
    MPS.publishValue(kSubStatusDigitalData, digitalData);
  }
}
