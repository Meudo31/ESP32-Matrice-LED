// Matrice LED 8x8 - étape 3 : animations image par image et texte défilant
//
// Fonctionnement :
//  - Tout ce que fait l'étape 2 (éditeur au doigt, dessins enregistrés) est toujours là.
//  - Onglet Animation : on dessine une image, on l'ajoute, on en dessine une autre...
//    jusqu'à 16 images, lues en boucle à la vitesse choisie. 4 animations enregistrables,
//    plus 4 animations prêtes (coeur qui bat, Pac-Man, clin d'oeil, onde).
//  - Onglet Texte : un message qui défile de droite à gauche, accents et emoji coeur compris.
//  - La matrice reprend ce qu'elle affichait (dessin, animation ou texte) après une coupure.
//  - Bouton : appui court = élément suivant (forme, animation...), appui long = mode suivant.

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "config.h"
#include "max7219.h"
#include "formes.h"
#include "animations.h"
#include "police.h"
#include "bouton.h"
#include "page.h"

#define NB_MAX_DESSINS    12
#define NB_MAX_ANIMATIONS 4
#define LONGUEUR_NOM      16
#define LONGUEUR_TEXTE    200   // en octets (une lettre accentuée en prend 2, le coeur 3)
#define DELAI_SAUVEGARDE  3000  // on attend que l'affichage ne bouge plus avant d'écrire en mémoire

enum Mode : uint8_t { MODE_DESSIN, MODE_ANIMATION, MODE_TEXTE, NB_MODES };
const char* NOMS_MODES[] = {"dessin", "animation", "texte"};

struct Dessin {
  char nom[LONGUEUR_NOM];
  uint8_t image[8];
};

struct Animation {
  char nom[LONGUEUR_NOM];
  uint16_t delaiMs;
  uint8_t nbImages;
  uint8_t images[NB_MAX_IMAGES][8];
};

Matrice8x8 matrice;
Bouton bouton;
WebServer serveur(80);
Preferences memoire;

Dessin dessins[NB_MAX_DESSINS];
uint8_t nbDessins = 0;
Animation animations[NB_MAX_ANIMATIONS];
uint8_t nbAnimations = 0;

Mode mode = MODE_DESSIN;
uint8_t imageCourante[8] = {0};
Animation animCourante;
char texte[LONGUEUR_TEXTE + 1] = TEXTE_PAR_DEFAUT;
uint16_t vitesseTexte = 80;     // ms entre deux décalages d'une colonne
uint8_t luminosite = 4;

uint8_t positionDessin = 0;     // pour le bouton : formes puis dessins enregistrés
uint8_t positionAnim = 0;       // pour le bouton : animations prêtes puis enregistrées
uint8_t imageAnim = 0;          // image de l'animation affichée en ce moment
uint32_t dernierPas = 0;

// Texte découpé en colonnes de pixels, avec 8 colonnes vides avant et après
uint8_t colonnes[8 + LONGUEUR_TEXTE * 6 + 8];
uint16_t nbColonnes = 0;
uint16_t positionTexte = 0;

bool aSauvegarder = false;
uint32_t derniereModif = 0;

// ---------- Conversions ----------
// Une image voyage sous forme de 16 caractères hexadécimaux (2 par ligne, de haut en bas).
// Une animation est simplement la suite de ses images, collées les unes aux autres.
String imageEnHex(const uint8_t image[8]) {
  char hex[17];
  for (uint8_t i = 0; i < 8; i++) sprintf(hex + i * 2, "%02X", image[i]);
  return String(hex);
}

bool hexEnImage(const char* hex, uint8_t image[8]) {
  for (uint8_t i = 0; i < 8; i++) {
    char octet[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
    if (!isxdigit(octet[0]) || !isxdigit(octet[1])) return false;
    image[i] = strtoul(octet, nullptr, 16);
  }
  return true;
}

bool hexEnImage(const String& hex, uint8_t image[8]) {
  return hex.length() == 16 && hexEnImage(hex.c_str(), image);
}

// Remplit une animation à partir de la chaîne hexadécimale reçue de la page
bool hexEnAnimation(const String& hex, uint16_t delaiMs, Animation& anim) {
  size_t nb = hex.length() / 16;
  if (hex.length() % 16 != 0 || nb == 0 || nb > NB_MAX_IMAGES) return false;
  for (uint8_t i = 0; i < nb; i++) {
    if (!hexEnImage(hex.c_str() + i * 16, anim.images[i])) return false;
  }
  anim.nbImages = nb;
  anim.delaiMs = constrain(delaiMs, 30, 5000);
  return true;
}

void nettoyerNom(const String& entree, char sortie[LONGUEUR_NOM]) {
  uint8_t n = 0;
  for (size_t i = 0; i < entree.length() && n < LONGUEUR_NOM - 1; i++) {
    char c = entree[i];
    if (isalnum((unsigned char)c) || c == ' ' || c == '-' || c == '_') sortie[n++] = c;
  }
  sortie[n] = '\0';
  if (n == 0) strcpy(sortie, "Sans nom");
}

// Le texte est libre : on échappe les guillemets et les barres obliques pour le JSON
String echapperJson(const char* s) {
  String r;
  for (; *s; s++) {
    if (*s == '"' || *s == '\\') r += '\\';
    if ((uint8_t)*s >= 0x20) r += *s;
  }
  return r;
}

String animationEnJson(const char* nom, uint16_t delaiMs, uint8_t nbImages, const uint8_t images[][8]) {
  String json = "{\"nom\":\"" + String(nom) + "\",\"delai\":" + delaiMs + ",\"images\":\"";
  for (uint8_t i = 0; i < nbImages; i++) json += imageEnHex(images[i]);
  return json + "\"}";
}

// ---------- Affichage ----------
void preparerTexte() {
  nbColonnes = 0;
  for (uint8_t i = 0; i < 8; i++) colonnes[nbColonnes++] = 0;
  size_t pos = 0;
  while (texte[pos] && nbColonnes + 6 + 8 <= sizeof(colonnes)) {
    uint8_t g = glyphePour(lireUtf8(texte, pos));
    for (uint8_t c = 0; c < 5; c++) colonnes[nbColonnes++] = pgm_read_byte(&POLICE_5X7[g][c]);
    colonnes[nbColonnes++] = 0;   // espace entre deux lettres
  }
  for (uint8_t i = 0; i < 8; i++) colonnes[nbColonnes++] = 0;
  positionTexte = 0;
}

// Fenêtre de 8 colonnes sur le texte, convertie en image de 8 lignes
void afficherFenetreTexte() {
  uint8_t image[8] = {0};
  for (uint8_t x = 0; x < 8; x++) {
    uint8_t col = colonnes[positionTexte + x];
    for (uint8_t y = 0; y < 8; y++) {
      if ((col >> y) & 1) image[y] |= 0x80 >> x;
    }
  }
  matrice.afficher(image);
}

void marquerModifie() {
  aSauvegarder = true;
  derniereModif = millis();
}

void changerMode(Mode nouveau) {
  mode = nouveau;
  imageAnim = 0;
  dernierPas = 0;               // le prochain tour de loop() affiche tout de suite
  if (mode == MODE_DESSIN) matrice.afficher(imageCourante);
  if (mode == MODE_TEXTE) preparerTexte();
  marquerModifie();
  Serial.printf("Mode %s\n", NOMS_MODES[mode]);
}

void afficherDessin(const uint8_t image[8]) {
  memcpy(imageCourante, image, 8);
  if (mode != MODE_DESSIN) changerMode(MODE_DESSIN);
  matrice.afficher(imageCourante);
  marquerModifie();
}

void lancerAnimationPrete(uint8_t numero) {
  const AnimationPrete& a = ANIMATIONS[numero];
  strcpy(animCourante.nom, "");
  strncat(animCourante.nom, a.nom, LONGUEUR_NOM - 1);
  animCourante.delaiMs = a.delaiMs;
  animCourante.nbImages = a.nbImages;
  memcpy(animCourante.images, a.images, sizeof(a.images));
  changerMode(MODE_ANIMATION);
}

// Fait avancer l'animation ou le texte, à appeler à chaque tour de loop()
void animer() {
  uint32_t maintenant = millis();
  if (mode == MODE_ANIMATION && animCourante.nbImages > 0) {
    if (dernierPas == 0 || maintenant - dernierPas >= animCourante.delaiMs) {
      dernierPas = maintenant;
      if (imageAnim >= animCourante.nbImages) imageAnim = 0;
      matrice.afficher(animCourante.images[imageAnim++]);
    }
  } else if (mode == MODE_TEXTE) {
    if (dernierPas == 0 || maintenant - dernierPas >= vitesseTexte) {
      dernierPas = maintenant;
      afficherFenetreTexte();
      if (++positionTexte + 8 > nbColonnes) positionTexte = 0;   // le texte recommence
    }
  }
}

// ---------- Mémoire ----------
void chargerMemoire() {
  memoire.begin("matrice", true);
  luminosite = memoire.getUChar("lum", luminosite);
  memoire.getBytes("courant", imageCourante, 8);
  nbDessins = memoire.getUChar("nb", 0);
  if (nbDessins > NB_MAX_DESSINS) nbDessins = 0;
  if (nbDessins > 0) memoire.getBytes("dessins", dessins, sizeof(Dessin) * nbDessins);
  nbAnimations = memoire.getUChar("nbanim", 0);
  if (nbAnimations > NB_MAX_ANIMATIONS) nbAnimations = 0;
  if (nbAnimations > 0) memoire.getBytes("anims", animations, sizeof(Animation) * nbAnimations);
  bool animTrouvee = memoire.getBytes("animcour", &animCourante, sizeof(Animation)) == sizeof(Animation);
  if (memoire.isKey("texte")) memoire.getString("texte", texte, sizeof(texte));
  vitesseTexte = memoire.getUShort("vitesse", vitesseTexte);
  uint8_t modeLu = memoire.getUChar("mode", MODE_DESSIN);
  memoire.end();

  if (!animTrouvee || animCourante.nbImages == 0 || animCourante.nbImages > NB_MAX_IMAGES) {
    lancerAnimationPrete(0);    // première mise en route : coeur qui bat
  }
  mode = modeLu < NB_MODES ? (Mode)modeLu : MODE_DESSIN;
  Serial.printf("%u dessin(s) et %u animation(s) en mémoire\n", nbDessins, nbAnimations);
}

void sauvegarderDessins() {
  memoire.begin("matrice", false);
  memoire.putUChar("nb", nbDessins);
  if (nbDessins > 0) memoire.putBytes("dessins", dessins, sizeof(Dessin) * nbDessins);
  memoire.end();
}

void sauvegarderAnimations() {
  memoire.begin("matrice", false);
  memoire.putUChar("nbanim", nbAnimations);
  if (nbAnimations > 0) memoire.putBytes("anims", animations, sizeof(Animation) * nbAnimations);
  memoire.end();
}

void sauvegarderCourant() {
  memoire.begin("matrice", false);
  memoire.putBytes("courant", imageCourante, 8);
  memoire.putBytes("animcour", &animCourante, sizeof(Animation));
  memoire.putString("texte", texte);
  memoire.putUShort("vitesse", vitesseTexte);
  memoire.putUChar("mode", mode);
  memoire.putUChar("lum", luminosite);
  memoire.end();
  Serial.println("Affichage courant sauvegardé");
}

// ---------- Page web ----------
void repondreErreur(const char* message) {
  serveur.send(400, "text/plain", message);
}

void routeEtat() {
  String json;
  json.reserve(6000);
  json += "{\"image\":\"" + imageEnHex(imageCourante) + "\",\"lum\":" + luminosite;
  json += ",\"mode\":\"" + String(NOMS_MODES[mode]) + "\"";
  json += ",\"texte\":\"" + echapperJson(texte) + "\",\"vitesse\":" + vitesseTexte;
  json += ",\"maxDessins\":" + String(NB_MAX_DESSINS) + ",\"maxAnimations\":" + NB_MAX_ANIMATIONS;
  json += ",\"maxImages\":" + String(NB_MAX_IMAGES);
  json += ",\"anim\":" + animationEnJson(animCourante.nom, animCourante.delaiMs, animCourante.nbImages, animCourante.images);
  json += ",\"formes\":[";
  for (uint8_t i = 0; i < NB_FORMES; i++) {
    if (i) json += ',';
    json += "{\"nom\":\"" + String(FORMES[i].nom) + "\",\"image\":\"" + imageEnHex(FORMES[i].image) + "\"}";
  }
  json += "],\"dessins\":[";
  for (uint8_t i = 0; i < nbDessins; i++) {
    if (i) json += ',';
    json += "{\"nom\":\"" + String(dessins[i].nom) + "\",\"image\":\"" + imageEnHex(dessins[i].image) + "\"}";
  }
  json += "],\"animationsPretes\":[";
  for (uint8_t i = 0; i < NB_ANIMATIONS; i++) {
    if (i) json += ',';
    json += animationEnJson(ANIMATIONS[i].nom, ANIMATIONS[i].delaiMs, ANIMATIONS[i].nbImages, ANIMATIONS[i].images);
  }
  json += "],\"animations\":[";
  for (uint8_t i = 0; i < nbAnimations; i++) {
    if (i) json += ',';
    json += animationEnJson(animations[i].nom, animations[i].delaiMs, animations[i].nbImages, animations[i].images);
  }
  json += "]}";
  serveur.send(200, "application/json", json);
}

// /afficher?image=0066FFFFFF7E3C18 : affiche un dessin (et passe en mode dessin)
void routeAfficher() {
  uint8_t image[8];
  if (!hexEnImage(serveur.arg("image"), image)) return repondreErreur("image invalide");
  afficherDessin(image);
  serveur.send(200, "text/plain", "ok");
}

void routeLuminosite() {
  luminosite = constrain(serveur.arg("valeur").toInt(), 0, 15);
  matrice.luminosite(luminosite);
  marquerModifie();
  serveur.send(200, "text/plain", "ok");
}

void routeEnregistrerDessin() {
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

void routeSupprimerDessin() {
  int numero = serveur.arg("numero").toInt();
  if (!serveur.hasArg("numero") || numero < 0 || numero >= nbDessins) return repondreErreur("numero invalide");
  for (uint8_t i = numero; i + 1 < nbDessins; i++) dessins[i] = dessins[i + 1];
  nbDessins--;
  sauvegarderDessins();
  routeEtat();
}

// /animation?images=...&delai=200 : joue une animation sans l'enregistrer
void routeJouerAnimation() {
  Animation anim;
  if (!hexEnAnimation(serveur.arg("images"), serveur.arg("delai").toInt(), anim)) return repondreErreur("animation invalide");
  nettoyerNom(serveur.hasArg("nom") ? serveur.arg("nom") : String("En cours"), anim.nom);
  animCourante = anim;
  changerMode(MODE_ANIMATION);
  serveur.send(200, "text/plain", "ok");
}

// /animation/enregistrer?nom=...&images=...&delai=200
void routeEnregistrerAnimation() {
  Animation anim;
  if (!hexEnAnimation(serveur.arg("images"), serveur.arg("delai").toInt(), anim)) return repondreErreur("animation invalide");
  nettoyerNom(serveur.arg("nom"), anim.nom);
  int8_t place = -1;
  for (uint8_t i = 0; i < nbAnimations; i++) {
    if (strcmp(animations[i].nom, anim.nom) == 0) place = i;
  }
  if (place < 0) {
    if (nbAnimations >= NB_MAX_ANIMATIONS) return repondreErreur("memoire pleine : supprime une animation");
    place = nbAnimations++;
  }
  animations[place] = anim;
  sauvegarderAnimations();
  animCourante = anim;
  changerMode(MODE_ANIMATION);
  Serial.printf("Animation enregistrée : %s (%u images)\n", anim.nom, anim.nbImages);
  routeEtat();
}

void routeSupprimerAnimation() {
  int numero = serveur.arg("numero").toInt();
  if (!serveur.hasArg("numero") || numero < 0 || numero >= nbAnimations) return repondreErreur("numero invalide");
  for (uint8_t i = numero; i + 1 < nbAnimations; i++) animations[i] = animations[i + 1];
  nbAnimations--;
  sauvegarderAnimations();
  routeEtat();
}

// /texte?texte=Bonjour&vitesse=80
void routeTexte() {
  String recu = serveur.arg("texte");
  if (recu.length() == 0) return repondreErreur("texte vide");
  if (recu.length() > LONGUEUR_TEXTE) return repondreErreur("texte trop long");
  strcpy(texte, recu.c_str());
  if (serveur.hasArg("vitesse")) vitesseTexte = constrain(serveur.arg("vitesse").toInt(), 20, 500);
  changerMode(MODE_TEXTE);
  Serial.printf("Texte : %s\n", texte);
  serveur.send(200, "text/plain", "ok");
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
    animer();                   // la matrice continue de vivre pendant la connexion
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connecté : ouvre http://%s.local ou http://%s\n", NOM_RESEAU, WiFi.localIP().toString().c_str());
  } else {
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
  serveur.on("/enregistrer", routeEnregistrerDessin);
  serveur.on("/supprimer", routeSupprimerDessin);
  serveur.on("/animation", routeJouerAnimation);
  serveur.on("/animation/enregistrer", routeEnregistrerAnimation);
  serveur.on("/animation/supprimer", routeSupprimerAnimation);
  serveur.on("/texte", routeTexte);
  serveur.onNotFound([]() { serveur.send(404, "text/plain", "Page introuvable"); });
  serveur.begin();
}

// ---------- Bouton ----------
void elementSuivant() {
  if (mode == MODE_DESSIN) {
    positionDessin = (positionDessin + 1) % (NB_FORMES + nbDessins);
    afficherDessin(positionDessin < NB_FORMES ? FORMES[positionDessin].image
                                              : dessins[positionDessin - NB_FORMES].image);
  } else if (mode == MODE_ANIMATION) {
    positionAnim = (positionAnim + 1) % (NB_ANIMATIONS + nbAnimations);
    if (positionAnim < NB_ANIMATIONS) {
      lancerAnimationPrete(positionAnim);
    } else {
      animCourante = animations[positionAnim - NB_ANIMATIONS];
      changerMode(MODE_ANIMATION);
    }
    Serial.printf("Animation : %s\n", animCourante.nom);
  } else {
    changerMode(MODE_TEXTE);    // le texte repart du début
  }
}

void gererBouton() {
  switch (bouton.lire()) {
    case COURT: elementSuivant(); break;
    case LONG:  changerMode((Mode)((mode + 1) % NB_MODES)); break;
    case AUCUN: break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Matrice LED 8x8 - étape 3 ===");

  matrice.begin(BROCHE_CS, ROTATION_MATRICE, MIROIR_MATRICE);
  chargerMemoire();
  matrice.luminosite(luminosite);
  changerMode(mode);
  aSauvegarder = false;         // rien n'a changé depuis la lecture de la mémoire
  bouton.begin(BROCHE_BOUTON);

  demarrerReseau();
  demarrerServeur();
}

void loop() {
  serveur.handleClient();
  gererBouton();
  animer();

  if (aSauvegarder && millis() - derniereModif > DELAI_SAUVEGARDE) {
    aSauvegarder = false;
    sauvegarderCourant();
  }
}
