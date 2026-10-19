# Bonus : pour aller plus loin

Idées de suites pour la chaîne, chacune peut faire une vidéo courte. Liste d'achat : voir la section Bonus du [README principal](../README.md#bonus-optionnel).

## 4 matrices chaînées (bandeau 32x8)

Le module « FC-16 » regroupe 4 matrices en ligne, reliées en chaîne par DOUT vers DIN. Même câblage que la matrice seule. Pour le piloter, on envoie 4 paires registre/valeur à la suite avant de remonter CS : la première paire part au bout de la chaîne. Le texte défilant devient enfin lisible d'un coup d'oeil, et l'éditeur passe à 32x8. La bibliothèque `MD_Parola` fait tout cela avec des effets de transition, si on ne veut pas l'écrire soi-même.

## Horloge et météo

Avec l'heure Internet (NTP, déjà utilisée par la guirlande de Noël), le bandeau 32x8 affiche l'heure, puis la température récupérée sur une API météo gratuite comme Open-Meteo.

## Notifications

Faire défiler un message reçu : une notification ntfy (comme la boîte aux lettres), un message MQTT envoyé par Home Assistant (« Lave-linge terminé »), ou un message envoyé depuis la page. La boîte aux lettres peut ainsi afficher « Courrier ! » dans l'entrée.

## Version couleur

Une matrice WS2812B 8x8 se pilote avec FastLED, comme la guirlande. L'éditeur garde la même page, avec une palette de couleurs en plus ; chaque image passe de 8 à 192 octets.

## Jeu Snake

Avec 4 boutons (ou ceux du module ludique pour enfants), la matrice 8x8 suffit pour un Snake : le serpent est une liste de cases, la pomme une case au hasard. Le score défile en texte à la fin de la partie.

## Sablier numérique

Avec un MPU6050 (celui du drone), les LED tombent comme du sable selon le sens dans lequel on tient la matrice. Très visuel en vidéo courte.
