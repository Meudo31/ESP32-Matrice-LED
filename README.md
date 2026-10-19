# Matrice LED 8x8 pilotée depuis le téléphone

Une matrice de 64 LED rouges sur laquelle on dessine au doigt depuis le téléphone, puis qu'on anime image par image et qui fait défiler des messages. Le projet avance par étapes, du plus simple au plus complet ; chaque étape est un projet PlatformIO autonome et peut faire l'objet d'une vidéo.

| Étape | Dossier | Ce que ça fait | Carte |
| --- | --- | --- | --- |
| 1 | [etape-1-formes](etape-1-formes) | Formes prêtes (coeur, sourire, Pac-Man...) changées au bouton, diaporama, luminosité | ESP32 DevKit V1 |
| 2 | [etape-2-editeur-web](etape-2-editeur-web) | Éditeur 8x8 sur la page http://matrice.local : on dessine au doigt, la matrice suit en direct, 12 dessins en mémoire | ESP32 DevKit V1 |
| 3 | [etape-3-animations](etape-3-animations) | Animations jusqu'à 16 images, texte défilant avec accents, reprise après coupure | ESP32 DevKit V1 |
| Bonus | [bonus](bonus) | 4 matrices chaînées, horloge, notifications, version couleur, jeu Snake | |

Le même montage sert aux trois étapes : on ne change que le programme. Le pilote du MAX7219 est écrit à la main (une cinquantaine de lignes dans `include/max7219.h`), sans bibliothèque à installer, ce qui permet de montrer en vidéo comment la puce fonctionne.

## Liste d'achat par étape

Prix approximatifs ; chaque étape réutilise ce qui a été acheté avant. Les liens ouvrent une recherche, car les fiches produits changent souvent : choisis un vendeur bien noté avec beaucoup de commandes. Amazon livre en 1 à 3 jours, AliExpress coûte environ moitié moins mais met 1 à 3 semaines.

### Étape 1 (environ 12 à 20 €, tout sert aux étapes 2 et 3)

| Pièce | Prix approx. | Où acheter |
| --- | --- | --- |
| ESP32 DevKit V1 (ESP32-WROOM-32, 30 broches) | 4 à 8 € | [Amazon](https://www.amazon.fr/s?k=ESP32+DevKit+V1+WROOM-32) · [AliExpress](https://www.aliexpress.com/w/wholesale-ESP32-DevKit-V1.html) |
| Module matrice 8x8 à MAX7219, déjà monté (le kit à souder marche aussi) | 2 à 5 € | [Amazon](https://www.amazon.fr/s?k=MAX7219+matrice+8x8+module) · [AliExpress](https://www.aliexpress.com/w/wholesale-MAX7219-8x8-dot-matrix-module.html) |
| Bouton poussoir 12 mm ou 6 mm (lot) | 1 à 2 € | [Amazon](https://www.amazon.fr/s?k=bouton+poussoir+12mm+tactile) · [AliExpress](https://www.aliexpress.com/w/wholesale-tactile-push-button-12mm.html) |
| Plaque d'essai + fils Dupont femelle-femelle et mâle-femelle | 4 à 6 € | [Amazon](https://www.amazon.fr/s?k=breadboard+fils+dupont+kit) · [AliExpress](https://www.aliexpress.com/w/wholesale-breadboard-dupont-kit.html) |
| Câble USB de données (micro-USB ou USB-C selon la carte) | 0 à 3 € | [Amazon](https://www.amazon.fr/s?k=c%C3%A2ble+micro+USB+donn%C3%A9es) |

### Étapes 2 et 3 (rien de plus)

Le Wi-Fi est dans l'ESP32 : il suffit d'un téléphone ou d'un ordinateur sur le même réseau.

### Bonus (optionnel)

| Pièce | Prix approx. | Où acheter |
| --- | --- | --- |
| Module 4 matrices en ligne MAX7219 « FC-16 » (32x8) | 6 à 10 € | [Amazon](https://www.amazon.fr/s?k=MAX7219+4+en+1+matrice+32x8) · [AliExpress](https://www.aliexpress.com/w/wholesale-MAX7219-4-in-1-dot-matrix.html) |
| Matrice couleur 8x8 WS2812B (version RGB) | 5 à 10 € | [Amazon](https://www.amazon.fr/s?k=matrice+WS2812B+8x8) · [AliExpress](https://www.aliexpress.com/w/wholesale-WS2812B-8x8-matrix.html) |
| 74AHCT125 (adaptateur 3,3 V vers 5 V, seulement si l'affichage fait n'importe quoi) | 1 à 2 € | [Amazon](https://www.amazon.fr/s?k=74AHCT125) · [AliExpress](https://www.aliexpress.com/w/wholesale-74AHCT125.html) |
| Plaque de plexiglas fumé (rend le rendu bien plus joli à filmer) | 3 à 6 € | [Amazon](https://www.amazon.fr/s?k=plexiglas+fum%C3%A9+3mm+petit) |

## Câblage (identique pour les trois étapes)

| Module MAX7219 | ESP32 DevKit V1 |
| --- | --- |
| VCC | VIN (5 V de l'USB) |
| GND | GND |
| DIN | GPIO23 |
| CS (ou LOAD) | GPIO5 |
| CLK | GPIO18 |

| Bouton | ESP32 DevKit V1 |
| --- | --- |
| Patte 1 | GPIO4 |
| Patte 2 | GND |

Le module se branche par le côté marqué **DIN** (entrée) ; le côté **DOUT** sert à chaîner d'autres matrices. Les 64 LED au maximum consomment environ 300 mA : le port USB d'un ordinateur suffit.

## Mise en route

Chaque dossier a son propre README avec le détail. Dans tous les cas : `pio run -t upload` puis `pio device monitor` depuis le dossier de l'étape. Aux étapes 2 et 3, copier d'abord `include/config.example.h` en `include/config.h` et remplir le Wi-Fi. Le fichier `config.h` est exclu du dépôt.

Le code des trois étapes compile avec PlatformIO mais n'a pas encore été testé sur une vraie carte.
