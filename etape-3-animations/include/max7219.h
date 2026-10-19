// Pilote minimal pour une matrice 8x8 à MAX7219 (module « FC-16 » ou module carré bleu/rouge).
// Pas de bibliothèque externe : le MAX7219 se commande avec quelques registres sur le bus SPI.
//
// Une image fait 8 octets, un par ligne, de haut en bas.
// Dans chaque octet, le bit 7 est le pixel de gauche et le bit 0 celui de droite.
// Exemple : 0b10000001 allume les deux pixels du bord sur cette ligne.
#pragma once
#include <Arduino.h>
#include <SPI.h>

class Matrice8x8 {
public:
  // rotation : 0, 1, 2 ou 3 quarts de tour dans le sens horaire.
  // miroir : retourne l'image gauche/droite (certains modules sont câblés à l'envers).
  void begin(uint8_t brocheCS, uint8_t rotation = 0, bool miroir = false) {
    _cs = brocheCS;
    _rotation = rotation & 3;
    _miroir = miroir;
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);
    SPI.begin();                  // VSPI de l'ESP32 : CLK = GPIO18, DIN (MOSI) = GPIO23
    ecrire(0x0F, 0);              // test d'affichage désactivé
    ecrire(0x09, 0);              // pas de décodage « 7 segments »
    ecrire(0x0B, 7);              // les 8 lignes sont actives
    ecrire(0x0A, 4);              // luminosité moyenne
    ecrire(0x0C, 1);              // sortie du mode veille
    effacer();
  }

  // 0 (faible) à 15 (maximum)
  void luminosite(uint8_t niveau) { ecrire(0x0A, min<uint8_t>(niveau, 15)); }

  // Met la puce en veille (matrice éteinte, l'image reste en mémoire) ou la rallume
  void veille(bool active) { ecrire(0x0C, active ? 0 : 1); }

  void effacer() {
    const uint8_t vide[8] = {0};
    afficher(vide);
  }

  void afficher(const uint8_t image[8]) {
    uint8_t sortie[8];
    transformer(image, sortie);
    for (uint8_t ligne = 0; ligne < 8; ligne++) ecrire(ligne + 1, sortie[ligne]);
  }

  // Lit un pixel d'une image : x = colonne (0 à gauche), y = ligne (0 en haut)
  static bool pixel(const uint8_t image[8], uint8_t x, uint8_t y) {
    return (image[y] >> (7 - x)) & 1;
  }

private:
  uint8_t _cs = 5;
  uint8_t _rotation = 0;
  bool _miroir = false;

  void ecrire(uint8_t registre, uint8_t valeur) {
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_cs, LOW);
    SPI.transfer(registre);
    SPI.transfer(valeur);
    digitalWrite(_cs, HIGH);
    SPI.endTransaction();
  }

  // Applique la rotation et le miroir pour que l'image s'affiche dans le bon sens
  void transformer(const uint8_t entree[8], uint8_t sortie[8]) {
    for (uint8_t y = 0; y < 8; y++) {
      uint8_t octet = 0;
      for (uint8_t x = 0; x < 8; x++) {
        uint8_t sx = x, sy = y;
        switch (_rotation) {
          case 1: sx = y;     sy = 7 - x; break;
          case 2: sx = 7 - x; sy = 7 - y; break;
          case 3: sx = 7 - y; sy = x;     break;
        }
        if (_miroir) sx = 7 - sx;
        if (pixel(entree, sx, sy)) octet |= 0x80 >> x;
      }
      sortie[y] = octet;
    }
  }
};
