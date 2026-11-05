# CapteurConnecté

Tous les payloads sont des structures `packed`, en little-endian.
`<DEVICE_NAME>` est défini dans `config.h`.

| Subject | Sens | Envoi | Champs | Taille |
|---|---|---|---|---|
| `hb_<DEVICE_NAME>` | capteur → IHM | toutes les 1 s depuis `loop()` | `healthState` : `uint8_t` (0 = OK, 1 = dégradé, 2 = défaut ; toujours 0 ici)<br>`heartbeatCounter` : `uint16_t` (sans unité, repasse à 0 après 65 535)<br>`uptimeMilliseconds` : `uint32_t` (ms depuis le démarrage) | 7 octets |
| `status_<DEVICE_NAME>_counter` | capteur → IHM | toutes les 2 s, à l'ouverture de session et après un reset | `value` : `uint32_t` (sans unité, nombre d'incréments) | 4 octets |
| `cmd_<DEVICE_NAME>_reset_counter` | IHM → capteur | à la demande de l'utilisateur, avec ACK | aucun (payload vide, message ignoré si taille ≠ 0) | 0 octet |