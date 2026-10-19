// Copier ce fichier en config.h et remplir les valeurs.
// config.h ne doit jamais être publié (il contient le mot de passe Wi-Fi).
#pragma once

#define WIFI_SSID       "NomDeTonWifi"
#define WIFI_MOTDEPASSE "MotDePasseWifi"

// Nom de la matrice sur le réseau : la page s'ouvre sur http://matrice.local
#define NOM_RESEAU      "matrice"

// Si le Wi-Fi ne répond pas, la matrice crée son propre réseau (page sur http://192.168.4.1).
// Mot de passe de 8 caractères minimum.
#define AP_NOM          "Matrice-LED"
#define AP_MOTDEPASSE   "bidouille"

// ---------- Matériel ----------
#define BROCHE_CS        5      // CS du module MAX7219 (DIN = GPIO23, CLK = GPIO18)
#define BROCHE_BOUTON    4      // bouton poussoir entre GPIO4 et GND
// Mêmes valeurs qu'à l'étape 1 si tu les as changées
#define ROTATION_MATRICE 0      // 0 à 3 quarts de tour dans le sens horaire
#define MIROIR_MATRICE   false
