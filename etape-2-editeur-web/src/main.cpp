// Matrice LED 8x8 - étape 2 : éditeur de dessins sur le téléphone
//
// Fonctionnement :
//  - La matrice se connecte au Wi-Fi et sert une page sur http://matrice.local
//    (ou sur son adresse IP, affichée dans le moniteur série).
//  - Sur la page : une grille de 8x8 cases. On dessine au doigt et la matrice suit en direct.
//  - Outils : effacer, remplir, inverser, décaler, tourner, miroir, luminosité.
//  - On peut enregistrer jusqu'à 12 dessins dans la mémoire de l'ESP32 et les rappeler.
//  - Sans Wi-Fi, la matrice crée son propre réseau « Matrice-LED » (page sur http://192.168.4.1).
//  - Le bouton de l'étape 1 marche toujours : il fait défiler les formes et les dessins enregistrés.
//  - Le dernier dessin affiché et la luminosité reviennent après une coupure de courant.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "config.h"
#include "max7219.h"
#include "formes.h"
#include "bouton.h"
#include "page.h"

#define NB_MAX_DESSINS    12
#define LONGUEUR_NOM      16
#define DELAI_SAUVEGARDE  3000  // on attend que le dessin ne bouge plus avant d'écrire en mémoire

struct Dessin {
  char nom[LONGUEUR_NOM];
  uint8_t image[8];
};

Matrice8x8 matrice;
Bouton bouton;
WebServer serveur(80);
Preferences memoire;

Dessin dessins[NB_MAX_DESSINS];
uint8_t nbDessins = 0;
uint8_t imageCourante[8] = {0};
uint8_t luminosite = 4;
uint8_t positionBouton = 0;     // parcourt les formes puis les dessins enregistrés
bool aSauvegarder = false;
uint32_t derniereModif = 0;

// ---------- Conversions ----------
// Une image voyage entre la page et l'ESP32 sous forme de 16 caractères hexadécimaux
// (2 par ligne, de haut en bas). Exemple : le coeur s'écrit 0066FFFFFF7E3C18.
String imageEnHex(const uint8_t image[8]) {
  char texte[17];
  for (uint8_t i = 0; i < 8; i++) sprintf(texte + i * 2, "%02X", image[i]);
  return String(texte);
}

bool hexEnImage(const String& texte, uint8_t image[8]) {
  if (texte.length() != 16) return false;
  for (uint8_t i = 0; i < 8; i++) {
    char octet[3] = {texte[i * 2], texte[i * 2 + 1], '\0'};
    if (!isxdigit(octet[0]) || !isxdigit(octet[1])) return false;
    image[i] = strtoul(octet, nullptr, 16);
  }
  return true;
}

// Garde seulement les caractères sans risque pour le JSON et la page
void nettoyerNom(const String& entree, char sortie[LONGUEUR_NOM]) {
  uint8_t n = 0;
  for (size_t i = 0; i < entree.length() && n < LONGUEUR_NOM - 1; i++) {
    char c = entree[i];
    if (isalnum((unsigned char)c) || c == ' ' || c == '-' || c == '_') sortie[n++] = c;
  }
  sortie[n] = '\0';
  if (n == 0) strcpy(sortie, "Sans nom");
}

// ---------- Mémoire ----------
void chargerMemoire() {
  memoire.begin("matrice", true);
  luminosite = memoire.getUChar("lum", luminosite);
  memoire.getBytes("courant", imageCourante, 8);
  nbDessins = memoire.getUChar("nb", 0);
  if (nbDessins > NB_MAX_DESSINS) nbDessins = 0;
  if (nbDessins > 0) memoire.getBytes("dessins", dessins, sizeof(Dessin) * nbDessins);
  memoire.end();
  Serial.printf("%u dessin(s) enregistré(s) en mémoire\n", nbDessins);
}

void sauvegarderDessins() {
  memoire.begin("matrice", false);
  memoire.putUChar("nb", nbDessins);
  if (nbDessins > 0) memoire.putBytes("dessins", dessins, sizeof(Dessin) * nbDessins);
  memoire.end();
}

void sauvegarderCourant() {
  memoire.begin("matrice", false);
  memoire.putBytes("courant", imageCourante, 8);
  memoire.putUChar("lum", luminosite);
  memoire.end();
  Serial.println("Dessin courant et luminosité sauvegardés");
}

void marquerModifie() {
  aSauvegarder = true;
  derniereModif = millis();
}

void afficher(const uint8_t image[8]) {
  memcpy(imageCourante, image, 8);
  matrice.afficher(imageCourante);
  marquerModifie();
}

// ---------- Page web ----------
void repondreErreur(const char* message) {
  serveur.send(400, "text/plain", message);
}

// Tout l'état en une fois : image affichée, luminosité, formes et dessins enregistrés
void routeEtat() {
  String json;
  json.reserve(1500);
  json += "{\"image\":\"" + imageEnHex(imageCourante) + "\",\"lum\":" + luminosite;
  json += ",\"max\":" + String(NB_MAX_DESSINS) + ",\"formes\":[";
  for (uint8_t i = 0; i < NB_FORMES; i++) {
    if (i) json += ',';
    json += "{\"nom\":\"" + String(FORMES[i].nom) + "\",\"image\":\"" + imageEnHex(FORMES[i].image) + "\"}";
  }
  json += "],\"dessins\":[";
  for (uint8_t i = 0; i < nbDessins; i++) {
    if (i) json += ',';
    json += "{\"nom\":\"" + String(dessins[i].nom) + "\",\"image\":\"" + imageEnHex(dessins[i].image) + "\"}";
  }
  json += "]}";
  serveur.send(200, "application/json", json);
}

// /afficher?image=0066FFFFFF7E3C18
void routeAfficher() {
  uint8_t image[8];
  if (!hexEnImage(serveur.arg("image"), image)) return repondreErreur("image invalide");
  afficher(image);
  serveur.send(200, "text/plain", "ok");
}

// /luminosite?valeur=0..15
void routeLuminosite() {
  int valeur = serveur.arg("valeur").toInt();
  luminosite = constrain(valeur, 0, 15);
  matrice.luminosite(luminosite);
  marquerModifie();
  serveur.send(200, "text/plain", "ok");
}

// /enregistrer?nom=Mon%20dessin&image=... : remplace un dessin du même nom s'il existe
void routeEnregistrer() {
  uint8_t image[8];
  if (!hexEnImage(serveur.arg("image"), image)) return repondreErreur("image invalide");
  char nom[LONGUEUR_NOM];
  nettoyerNom(serveur.arg("nom"), nom);

  int8_t place = -1;
  for (uint8_t i = 0; i < nbDessins; i++) {
    if (strcmp(dessins[i].nom, nom) == 0) place = i;
  }
  if (place < 0) {
    if (nbDessins >= NB_MAX_DESSINS) return repondreErreur("memoire pleine : supprime un dessin");
    place = nbDessins++;
  }
  strcpy(dessins[place].nom, nom);
  memcpy(dessins[place].image, image, 8);
  sauvegarderDessins();
  Serial.printf("Dessin enregistré : %s\n", nom);
  routeEtat();
}

// /supprimer?numero=3
void routeSupprimer() {
  int numero = serveur.arg("numero").toInt();
  if (!serveur.hasArg("numero") || numero < 0 || numero >= nbDessins) return repondreErreur("numero invalide");
  Serial.printf("Dessin supprimé : %s\n", dessins[numero].nom);
  for (uint8_t i = numero; i + 1 < nbDessins; i++) dessins[i] = dessins[i + 1];
  nbDessins--;
  sauvegarderDessins();
  routeEtat();
}

// ---------- Wi-Fi ----------
void demarrerReseau() {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(NOM_RESEAU);
  WiFi.begin(WIFI_SSID, WIFI_MOTDEPASSE);
  Serial.printf("Connexion au Wi-Fi %s", WIFI_SSID);
  uint32_t debut = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - debut < 15000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connecté : ouvre http://%s.local ou http://%s\n", NOM_RESEAU, WiFi.localIP().toString().c_str());
  } else {
    // Pas de Wi-Fi : la matrice devient son propre point d'accès
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_NOM, AP_MOTDEPASSE);
    Serial.printf("Wi-Fi introuvable : connecte-toi au réseau %s puis ouvre http://%s\n",
                  AP_NOM, WiFi.softAPIP().toString().c_str());
  }
  if (MDNS.begin(NOM_RESEAU)) MDNS.addService("http", "tcp", 80);
}

void demarrerServeur() {
  serveur.on("/", []() { serveur.send_P(200, "text/html", PAGE_HTML); });
  serveur.on("/etat", routeEtat);
  serveur.on("/afficher", routeAfficher);
  serveur.on("/luminosite", routeLuminosite);
  serveur.on("/enregistrer", routeEnregistrer);
  serveur.on("/supprimer", routeSupprimer);
  serveur.onNotFound([]() { serveur.send(404, "text/plain", "Page introuvable"); });
  serveur.begin();
}

// ---------- Bouton ----------
// Appui court : forme ou dessin suivant. Appui long : luminosité suivante.
void gererBouton() {
  switch (bouton.lire()) {
    case COURT: {
      uint8_t total = NB_FORMES + nbDessins;
      positionBouton = (positionBouton + 1) % total;
      if (positionBouton < NB_FORMES) {
        afficher(FORMES[positionBouton].image);
        Serial.printf("Bouton : forme %s\n", FORMES[positionBouton].nom);
      } else {
        const Dessin& d = dessins[positionBouton - NB_FORMES];
        afficher(d.image);
        Serial.printf("Bouton : dessin %s\n", d.nom);
      }
      break;
    }
    case LONG:
      luminosite = luminosite >= 15 ? 1 : (luminosite < 4 ? 4 : (luminosite < 8 ? 8 : 15));
      matrice.luminosite(luminosite);
      marquerModifie();
      Serial.printf("Luminosité : %u/15\n", luminosite);
      break;
    case AUCUN:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Matrice LED 8x8 - étape 2 ===");

  chargerMemoire();
  matrice.begin(BROCHE_CS, ROTATION_MATRICE, MIROIR_MATRICE);
  matrice.luminosite(luminosite);
  matrice.afficher(imageCourante);
  bouton.begin(BROCHE_BOUTON);

  demarrerReseau();
  demarrerServeur();
}

void loop() {
  serveur.handleClient();
  gererBouton();

  // Écriture différée : on ne fatigue pas la mémoire flash à chaque coup de doigt
  if (aSauvegarder && millis() - derniereModif > DELAI_SAUVEGARDE) {
    aSauvegarder = false;
    sauvegarderCourant();
  }
}
