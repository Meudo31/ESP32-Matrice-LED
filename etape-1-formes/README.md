# Étape 1 : des formes au bouton (sans Wi-Fi)

Liste d'achat et câblage : voir le [README principal](../README.md#liste-dachat-par-étape).

La matrice affiche 14 formes prêtes : coeur, sourire, triste, flèches, coche, croix, envahisseur, Pac-Man, fantôme, note, étoile, maison et damier.

- **Appui court** : forme suivante.
- **Appui long** (près d'une seconde) : luminosité suivante, sur 4 niveaux.
- **Sans appui pendant 10 s** : diaporama, une forme toutes les 2 secondes.

Au démarrage, la matrice s'allume ligne par ligne : c'est un test qui montre tout de suite si une LED est morte. Chaque forme est aussi dessinée en `#` et `.` dans le moniteur série.

## Mise en route

1. Câble le module et le bouton comme dans le README principal.
2. Branche l'ESP32 en USB et lance `pio run -t upload`, puis `pio device monitor`. Si le téléversement reste bloqué sur « Connecting... », maintiens le bouton BOOT de la carte quelques secondes.
3. Compare la matrice au dessin du moniteur série.

## Si l'image est de travers

Selon le fabricant, la matrice est soudée dans un sens ou dans l'autre sur son circuit. Tout se règle en haut de `src/main.cpp` :

- l'image est tournée : change `ROTATION_MATRICE` (0, 1, 2 ou 3 quarts de tour) ;
- l'image est inversée gauche/droite : mets `MIROIR_MATRICE` à `true`.

Note les valeurs qui marchent : tu les reporteras dans `config.h` aux étapes 2 et 3.

## Créer ses propres formes

Les formes sont dans `include/formes.h`, écrites ligne par ligne en binaire : un `1` est une LED allumée. Recopie un bloc, change les 0 et les 1, donne-lui un nom, et la forme apparaît au bouton. C'est un bon moment pour la vidéo : on voit le dessin directement dans le code.

## Comment ça marche

Le MAX7219 garde en mémoire l'état des 64 LED et les allume lui-même en balayant les lignes très vite. L'ESP32 lui envoie simplement 8 octets, un par ligne, par le bus SPI (DIN = données, CLK = horloge, CS = « c'est à toi »). Tout le pilote tient dans `include/max7219.h`.

## Points à vérifier ensemble

- **Affichage instable ou LED au hasard** : le MAX7219 alimenté en 5 V lit en général bien les signaux 3,3 V de l'ESP32, mais pas toujours. Raccourcis d'abord les fils ; si ça ne suffit pas, passe DIN, CLK et CS par un 74AHCT125 (le même que pour la guirlande de Noël).
- **Luminosité** : au niveau 15, la matrice éblouit la caméra. Pour filmer, le niveau 1 ou 4 rend mieux.
