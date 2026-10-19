// Matrice LED 8x8 - étape 1 : afficher des formes et les changer au bouton
//
// Fonctionnement :
//  - Au démarrage, un petit test allume la matrice ligne par ligne.
//  - Appui court sur le bouton : forme suivante (coeur, sourire, flèches, Pac-Man...).
//  - Appui long : change la luminosité (4 niveaux).
//  - Sans appui pendant 10 s, la matrice passe en diaporama et change de forme toute seule.
//  - Chaque forme est aussi dessinée dans le moniteur série, pratique pour vérifier le sens.

#include <Arduino.h>
#include "max7219.h"
#include "formes.h"
#include "bouton.h"

// ---------- Matériel ----------
#define BROCHE_CS        5   // CS du module MAX7219 (DIN = GPIO23, CLK = GPIO18)
#define BROCHE_BOUTON    4   // bouton poussoir entre GPIO4 et GND
// Si l'image s'affiche de travers, change ces deux réglages (rotation : 0 à 3 quarts de tour)
#define ROTATION_MATRICE 0
#define MIROIR_MATRICE   false

#define DIAPORAMA_APRES_MS 10000
#define DIAPORAMA_PAS_MS   2000

const uint8_t NIVEAUX_LUMINOSITE[] = {1, 4, 8, 15};

Matrice8x8 matrice;
Bouton bouton;
uint8_t formeCourante = 0;
uint8_t niveauLuminosite = 1;
uint32_t dernierAppui = 0;
uint32_t dernierChangement = 0;

// Dessine l'image dans le moniteur série avec des # et des .
void dessinerDansLeMoniteur(const uint8_t image[8]) {
  for (uint8_t y = 0; y < 8; y++) {
    char ligne[9];
    for (uint8_t x = 0; x < 8; x++) ligne[x] = Matrice8x8::pixel(image, x, y) ? '#' : '.';
    ligne[8] = '\0';
    Serial.println(ligne);
  }
}

void afficherForme(uint8_t numero) {
  formeCourante = numero % NB_FORMES;
  matrice.afficher(FORMES[formeCourante].image);
  dernierChangement = millis();
  Serial.printf("\nForme %u/%u : %s\n", formeCourante + 1, NB_FORMES, FORMES[formeCourante].nom);
  dessinerDansLeMoniteur(FORMES[formeCourante].image);
}

// Allume la matrice ligne par ligne : permet de vérifier que les 64 LED marchent
void testDemarrage() {
  uint8_t image[8] = {0};
  for (uint8_t y = 0; y < 8; y++) {
    image[y] = 0xFF;
    matrice.afficher(image);
    delay(80);
  }
  delay(300);
  matrice.effacer();
  delay(200);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Matrice LED 8x8 - étape 1 ===");

  matrice.begin(BROCHE_CS, ROTATION_MATRICE, MIROIR_MATRICE);
  matrice.luminosite(NIVEAUX_LUMINOSITE[niveauLuminosite]);
  bouton.begin(BROCHE_BOUTON);

  testDemarrage();
  afficherForme(0);
  dernierAppui = millis();
}

void loop() {
  uint32_t maintenant = millis();

  switch (bouton.lire()) {
    case COURT:
      dernierAppui = maintenant;
      afficherForme(formeCourante + 1);
      break;
    case LONG:
      dernierAppui = maintenant;
      niveauLuminosite = (niveauLuminosite + 1) % sizeof(NIVEAUX_LUMINOSITE);
      matrice.luminosite(NIVEAUX_LUMINOSITE[niveauLuminosite]);
      Serial.printf("Luminosité : %u/15\n", NIVEAUX_LUMINOSITE[niveauLuminosite]);
      break;
    case AUCUN:
      break;
  }

  // Diaporama quand personne ne touche au bouton
  if (maintenant - dernierAppui > DIAPORAMA_APRES_MS &&
      maintenant - dernierChangement > DIAPORAMA_PAS_MS) {
    afficherForme(formeCourante + 1);
  }
}
