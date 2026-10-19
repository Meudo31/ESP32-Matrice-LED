// Bouton poussoir entre une broche et GND, avec anti-rebond, appui court et appui long.
#pragma once
#include <Arduino.h>

enum Appui { AUCUN, COURT, LONG };

class Bouton {
public:
  void begin(uint8_t broche, uint16_t dureeLongMs = 800) {
    _broche = broche;
    _dureeLong = dureeLongMs;
    pinMode(_broche, INPUT_PULLUP);
  }

  // À appeler à chaque tour de loop()
  Appui lire() {
    bool enfonce = digitalRead(_broche) == LOW;
    uint32_t maintenant = millis();
    if (enfonce != _etatBrut) {
      _etatBrut = enfonce;
      _dernierChangement = maintenant;
    }
    if (maintenant - _dernierChangement < 30) return AUCUN;   // anti-rebond

    if (enfonce && !_enfonce) {
      _enfonce = true;
      _debutAppui = maintenant;
      _longSignale = false;
    } else if (enfonce && !_longSignale && maintenant - _debutAppui >= _dureeLong) {
      _longSignale = true;            // l'appui long est signalé sans attendre le relâchement
      return LONG;
    } else if (!enfonce && _enfonce) {
      _enfonce = false;
      if (!_longSignale) return COURT;
    }
    return AUCUN;
  }

private:
  uint8_t _broche = 4;
  uint16_t _dureeLong = 800;
  bool _etatBrut = false, _enfonce = false, _longSignale = false;
  uint32_t _dernierChangement = 0, _debutAppui = 0;
};
