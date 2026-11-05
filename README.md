# CapteurConnecté

Tous les payloads sont des structures `packed`, en little-endian.
`<DEVICE_NAME>` est défini dans `config.h`.

| Subject | Sens | Envoi | Champs | Taille |
|---|---|---|---|---|
| `hb_<DEVICE_NAME>` | capteur → IHM | toutes les 1 s depuis `loop()` | `healthState` : `uint8_t` (0 = OK, 1 = dégradé, 2 = défaut ; toujours 0 ici)<br>`heartbeatCounter` : `uint16_t` (sans unité, repasse à 0 après 65 535)<br>`uptimeMilliseconds` : `uint32_t` (ms depuis le démarrage) | 7 octets |
| `status_<DEVICE_NAME>_counter` | capteur → IHM | toutes les 2 s, à l'ouverture de session et après un reset | `value` : `uint32_t` (sans unité, nombre d'incréments) | 4 octets |
| `cmd_<DEVICE_NAME>_reset_counter` | IHM → capteur | à la demande de l'utilisateur, avec ACK | aucun (payload vide, message ignoré si taille ≠ 0) | 0 octet |
| `status_<DEVICE_NAME>_analog_data` | capteur → IHM | toutes les 1s et à l’ouverture de session | `value` : `uint32_t`, sans unité : valeur brute de l'ADC 12 bits de l'ESP32, de 0 (0 V) à 4095 (≈ 3,3 V), lue sur `kAnalogPin`  | 4 octets |
| `status_<DEVICE_NAME>_digital_data` | capteur → IHM | toutes les 1s et à l’ouverture de session | `value` : `uint8_t`, sans unité : état logique de `kDigitalPin`, 0 ou 1. L'entrée est en `INPUT_PULLUP` : 1 = repos (contact ouvert), 0 = actif (contact fermé vers la masse)  | 1 octet |
| `status_<DEVICE_NAME>_temperature_data` | capteur → IHM | toutes les 1s et à l’ouverture de session | `value` : `float` (IEEE 754), en °C, mesurée par le DHT11 (plage 0 à 50 °C, précision ± 2 °C)  | 4 octets |
| `status_<DEVICE_NAME>_humidity_data` | capteur → IHM | toutes les 1s et à l’ouverture de session | `value` : `float` (IEEE 754), en % d'humidité relative, mesurée par le DHT11 (plage 20 à 90 %, précision ± 5 %)  | 4 octets |




| `status_<DEVICE_NAME>_temperature_data` | capteur → IHM | toutes les 1 s et à l'ouverture de session | `value` : `float` (IEEE 754), en °C, mesurée par le DHT11 (plage 0 à 50 °C, précision ± 2 °C) | 4 octets |
| `status_<DEVICE_NAME>_humidity_data` | capteur → IHM | toutes les 1 s et à l'ouverture de session | `value` : `float` (IEEE 754), en % d'humidité relative, mesurée par le DHT11 (plage 20 à 90 %, précision ± 5 %) | 4 octets |