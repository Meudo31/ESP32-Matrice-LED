# Étape 3 : animations et texte défilant

Liste d'achat et câblage : voir le [README principal](../README.md#liste-dachat-par-étape). Rien de plus qu'à l'étape 1.

La page **http://matrice.local** garde l'éditeur de l'étape 2 et gagne trois onglets.

- **Dessins** : comme à l'étape 2 (12 dessins en mémoire, formes prêtes).
- **Animation** : on dessine une image, on touche « Ajouter le dessin », on modifie, on ajoute encore... jusqu'à 16 images. Toucher une image la recharge dans l'éditeur pour la retoucher (« Remplacer ») ou la retirer. Le curseur règle le temps de chaque image (30 ms à 1 s), puis « Lire sur la matrice » la joue en boucle. 4 animations enregistrables, plus 4 prêtes : coeur qui bat, Pac-Man, clin d'oeil, onde.
- **Texte** : un message de 60 caractères qui défile de droite à gauche. Les accents sont acceptés (ils s'affichent sans accent, 7 pixels de haut c'est trop petit), ainsi que `°`, `€` et le coeur `♥`. Le curseur règle la vitesse (à droite = plus vite).

Après une coupure de courant, la matrice reprend exactement ce qu'elle affichait : dessin, animation ou texte. La toute première fois, elle fait défiler « Bidouille Connectée ♥ » (modifiable dans `config.h`).

## Bouton

- **Appui court** : élément suivant du mode en cours (forme ou dessin suivant, animation suivante, ou texte qui repart du début).
- **Appui long** : mode suivant (dessin, animation, texte). La luminosité se règle maintenant depuis la page.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h`, remplis le Wi-Fi et reporte la rotation et le miroir de l'étape 1.
2. Lance `pio run -t upload`, puis `pio device monitor`.
3. Ouvre **http://matrice.local** (ou l'adresse IP affichée). Sans Wi-Fi, même réseau de secours qu'à l'étape 2 : **Matrice-LED**, page sur http://192.168.4.1.

Les dessins enregistrés à l'étape 2 sont retrouvés, car les deux étapes utilisent la même mémoire.

## Comment ça marche

- **Animation** : simplement la suite de ses images en hexadécimal (16 caractères par image), envoyée à `/animation?images=...&delai=...`. L'ESP32 change d'image quand le délai est écoulé, sans `delay()`, pour que la page et le bouton restent réactifs.
- **Texte défilant** : chaque lettre de la police 5x7 (`include/police.h`) donne 5 colonnes de pixels, plus une colonne vide. Le texte devient une longue bande de colonnes, et la matrice n'en montre que 8 à la fois. On décale la fenêtre d'une colonne à chaque pas.
- **Accents** : la page envoie le texte en UTF-8 ; `lireUtf8()` reconstitue chaque caractère et `glyphePour()` choisit la lettre à afficher.

## Points à vérifier ensemble

- **Texte à l'envers ou en miroir** : ce sont les mêmes réglages `ROTATION_MATRICE` et `MIROIR_MATRICE` que pour les dessins. Si les dessins s'affichent bien, le texte aussi.
- **Lisibilité** : une seule matrice ne montre qu'une lettre et demie à la fois, il faut une vitesse assez lente (vers 80 à 120 ms). Pour un vrai bandeau, voir le bonus avec 4 matrices.
