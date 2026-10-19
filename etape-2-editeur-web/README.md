# Étape 2 : l'éditeur de dessins sur le téléphone

Liste d'achat et câblage : voir le [README principal](../README.md#liste-dachat-par-étape). Rien de plus qu'à l'étape 1.

La matrice sert une page web sur **http://matrice.local**. On y trouve une grille de 8x8 cases : on dessine au doigt et la matrice suit en direct.

- **Dessiner** : touche une case pour l'allumer, glisse le doigt pour tracer. Si la première case touchée était allumée, le doigt devient une gomme.
- **Outils** : effacer, remplir, inverser, tourner d'un quart de tour, miroir, et décaler le dessin dans les 4 directions (ce qui sort d'un côté revient de l'autre).
- **Luminosité** : un curseur de 0 à 15.
- **Enregistrer** : jusqu'à 12 dessins nommés, gardés dans la mémoire de l'ESP32 même sans courant. Un nom déjà pris remplace l'ancien dessin.
- **Formes prêtes** : les 14 formes de l'étape 1, à toucher pour les charger et les modifier.

Le dernier dessin affiché et la luminosité reviennent après une coupure de courant. Le bouton marche toujours : appui court pour passer aux formes puis aux dessins enregistrés, appui long pour la luminosité.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h`, remplis le Wi-Fi, et reporte `ROTATION_MATRICE` et `MIROIR_MATRICE` trouvés à l'étape 1.
2. Lance `pio run -t upload`, puis `pio device monitor`.
3. Le moniteur affiche l'adresse de la page. Ouvre **http://matrice.local** sur le téléphone (même Wi-Fi), ou l'adresse IP affichée si `.local` ne répond pas (c'est fréquent sur Android).

**Sans Wi-Fi** (salon, atelier, tournage en extérieur) : au bout de 15 secondes sans connexion, la matrice crée son propre réseau **Matrice-LED** (mot de passe `bidouille`, modifiable dans `config.h`). Connecte le téléphone à ce réseau et ouvre **http://192.168.4.1**.

## Comment ça marche

- La page est stockée dans le programme (`src/page.h`) et envoyée par le petit serveur web de l'ESP32.
- Un dessin voyage sous forme de 16 caractères hexadécimaux, 2 par ligne. Le coeur s'écrit par exemple `0066FFFFFF7E3C18`. La page envoie `/afficher?image=...` à chaque coup de doigt (au plus toutes les 60 ms).
- Les dessins sont enregistrés avec la bibliothèque `Preferences` de l'ESP32. L'écriture du dessin courant attend 3 secondes de calme pour ne pas user la mémoire flash.

## Points à vérifier ensemble

- **matrice.local introuvable** : utilise l'adresse IP du moniteur série, ou réserve-la dans ta box pour qu'elle ne change plus.
- **Décalage entre le doigt et la matrice** : normalement imperceptible. Si le Wi-Fi est faible, rapproche la carte de la box.
