// Page web de la matrice, servie par l'ESP32 sur http://matrice.local
#pragma once
#include <Arduino.h>

const char PAGE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Matrice LED</title>
<style>
body{font-family:system-ui,sans-serif;background:#16181d;color:#eee;margin:0;padding:16px;max-width:460px;margin:auto}
h1{font-size:1.3em;margin:0 0 12px}
h2{font-size:1em;margin:20px 0 8px;color:#aaa}
#grille{display:grid;grid-template-columns:repeat(8,1fr);gap:6px;background:#000;padding:10px;border-radius:12px;touch-action:none;user-select:none}
#grille div{aspect-ratio:1;border-radius:50%;background:#2a0d0d}
#grille div.on{background:#ff3030;box-shadow:0 0 10px #ff3030}
.outils{display:flex;flex-wrap:wrap;gap:6px;margin-top:10px;align-items:center}
button{background:#2c3038;color:#eee;border:0;border-radius:8px;padding:10px 12px;font-size:1em}
button:active{background:#454b57}
button.principal{background:#b02828}
input[type=text]{flex:1;min-width:0;background:#2c3038;color:#eee;border:0;border-radius:8px;padding:10px;font-size:1em}
input[type=range]{flex:1}
.onglets{display:flex;gap:4px;margin-top:20px}
.onglets button{flex:1;border-radius:8px 8px 0 0;background:#22252b}
.onglets button.actif{background:#3a3f4a;font-weight:bold}
.panneau{display:none;background:#22252b;border-radius:0 0 8px 8px;padding:10px}
.panneau.actif{display:block}
.liste{display:grid;grid-template-columns:repeat(auto-fill,minmax(70px,1fr));gap:8px}
.vignette{background:#2c3038;border-radius:8px;padding:6px;text-align:center;font-size:.75em;position:relative}
.vignette.choisie{outline:2px solid #ff3030}
.mini{display:grid;grid-template-columns:repeat(8,1fr);gap:1px;margin-bottom:4px}
.mini i{aspect-ratio:1;background:#444}
.mini i.on{background:#ff3030}
.suppr{position:absolute;top:-6px;right:-6px;padding:2px 7px;border-radius:50%;background:#7a2020;font-size:.9em}
#message{min-height:1.2em;color:#ffb347;margin-top:10px}
</style></head><body>
<h1>Matrice LED 8x8</h1>
<div id="grille"></div>
<div class="outils">
<button onclick="outil('effacer')">Effacer</button>
<button onclick="outil('remplir')">Remplir</button>
<button onclick="outil('inverser')">Inverser</button>
<button onclick="outil('tourner')">Tourner</button>
<button onclick="outil('miroir')">Miroir</button>
<button onclick="decaler(-1,0)">&larr;</button>
<button onclick="decaler(1,0)">&rarr;</button>
<button onclick="decaler(0,-1)">&uarr;</button>
<button onclick="decaler(0,1)">&darr;</button>
</div>
<div class="outils">Luminosité <input type="range" id="lum" min="0" max="15" oninput="fetch('/luminosite?valeur='+this.value)"></div>
<div id="message"></div>

<div class="onglets">
<button data-p="dessins" class="actif">Dessins</button>
<button data-p="animation">Animation</button>
<button data-p="texte">Texte</button>
</div>

<div class="panneau actif" id="dessins">
<div class="outils"><input type="text" id="nomDessin" maxlength="15" placeholder="Nom du dessin"><button class="principal" onclick="enregistrerDessin()">Enregistrer</button></div>
<h2>Mes dessins</h2><div class="liste" id="listeDessins"></div>
<h2>Formes prêtes</h2><div class="liste" id="listeFormes"></div>
</div>

<div class="panneau" id="animation">
<h2>Images de l'animation (<span id="nbImages">0</span>)</h2>
<div class="liste" id="images"></div>
<div class="outils">
<button onclick="ajouterImage()">+ Ajouter le dessin</button>
<button onclick="remplacerImage()">Remplacer</button>
<button onclick="supprimerImage()">Retirer</button>
</div>
<div class="outils">Vitesse <input type="range" id="delai" min="30" max="1000" step="10" oninput="majDelai()"><span id="delaiTexte"></span></div>
<div class="outils"><button class="principal" onclick="jouer()">&#9654; Lire sur la matrice</button></div>
<div class="outils"><input type="text" id="nomAnim" maxlength="15" placeholder="Nom de l'animation"><button onclick="enregistrerAnim()">Enregistrer</button></div>
<h2>Mes animations</h2><div class="liste" id="listeAnims"></div>
<h2>Animations prêtes</h2><div class="liste" id="listePretes"></div>
</div>

<div class="panneau" id="texte">
<div class="outils"><input type="text" id="message_texte" maxlength="60" placeholder="Ton message"></div>
<div class="outils">Vitesse <input type="range" id="vitesse" min="20" max="300" step="5" style="direction:rtl"></div>
<div class="outils"><button class="principal" onclick="defiler()">Faire défiler</button></div>
</div>

<script>
let px=new Array(64).fill(0),crayon=null,attente=null,etat=null;
let anim=[],choisie=-1;
const $=id=>document.getElementById(id);
const grille=$('grille');
for(let i=0;i<64;i++){const d=document.createElement('div');d.dataset.i=i;grille.appendChild(d);}

// ----- Éditeur -----
function caseSous(e){const el=document.elementFromPoint(e.clientX,e.clientY);return el&&el.parentNode===grille?+el.dataset.i:-1;}
grille.addEventListener('pointerdown',e=>{const i=caseSous(e);if(i<0)return;crayon=px[i]?0:1;poser(i);});
grille.addEventListener('pointermove',e=>{if(crayon===null)return;const i=caseSous(e);if(i>=0)poser(i);});
addEventListener('pointerup',()=>crayon=null);
function poser(i){if(px[i]===crayon)return;px[i]=crayon;dessiner();envoyer();}
function dessiner(){grille.childNodes.forEach((d,i)=>d.classList.toggle('on',!!px[i]));}
function enHex(p){let s='';for(let y=0;y<8;y++){let o=0;for(let x=0;x<8;x++)if(p[y*8+x])o|=128>>x;s+=o.toString(16).padStart(2,'0');}return s.toUpperCase();}
function depuisHex(h){const p=[];for(let y=0;y<8;y++){const o=parseInt(h.substr(y*2,2),16);for(let x=0;x<8;x++)p.push((o>>(7-x))&1);}return p;}
function decouper(h){const r=[];for(let i=0;i<h.length;i+=16)r.push(h.substr(i,16));return r;}
function envoyer(){if(attente)return;attente=setTimeout(()=>{attente=null;fetch('/afficher?image='+enHex(px));},60);}
function charger(h){px=depuisHex(h);dessiner();envoyer();}
function outil(nom){
  if(nom=='effacer')px.fill(0);
  if(nom=='remplir')px.fill(1);
  if(nom=='inverser')px=px.map(v=>1-v);
  if(nom=='tourner'){const n=[];for(let y=0;y<8;y++)for(let x=0;x<8;x++)n.push(px[(7-x)*8+y]);px=n;}
  if(nom=='miroir'){const n=[];for(let y=0;y<8;y++)for(let x=0;x<8;x++)n.push(px[y*8+7-x]);px=n;}
  dessiner();envoyer();
}
function decaler(dx,dy){const n=[];for(let y=0;y<8;y++)for(let x=0;x<8;x++)n.push(px[((y-dy+8)%8)*8+(x-dx+8)%8]);px=n;dessiner();envoyer();}
function message(t){$('message').textContent=t;}
function requete(url,suite){fetch(url).then(r=>r.ok?(suite?r.json().then(suite):null):r.text().then(message));}

// ----- Onglets -----
document.querySelectorAll('.onglets button').forEach(b=>b.onclick=()=>{
  document.querySelectorAll('.onglets button,.panneau').forEach(e=>e.classList.remove('actif'));
  b.classList.add('actif');$(b.dataset.p).classList.add('actif');
});

// ----- Vignettes -----
function mini(h){const m=document.createElement('div');m.className='mini';
  depuisHex(h).forEach(b=>{const c=document.createElement('i');if(b)c.className='on';m.appendChild(c);});return m;}
function vignette(nom,h,clic,suppr){
  const v=document.createElement('div');v.className='vignette';
  v.appendChild(mini(h));v.appendChild(document.createTextNode(nom));v.onclick=clic;
  if(suppr){const s=document.createElement('button');s.className='suppr';s.textContent='×';
    s.onclick=e=>{e.stopPropagation();if(confirm('Supprimer « '+nom+' » ?'))suppr();};v.appendChild(s);}
  return v;
}

// ----- Dessins -----
function enregistrerDessin(){
  const nom=$('nomDessin').value.trim();
  if(!nom){message('Donne un nom au dessin.');return;}
  requete('/enregistrer?nom='+encodeURIComponent(nom)+'&image='+enHex(px),majListes);
}

// ----- Animation -----
function majImages(){
  const l=$('images');l.innerHTML='';
  anim.forEach((h,i)=>{const v=vignette(String(i+1),h,()=>{choisie=i;charger(h);majImages();});
    if(i==choisie)v.classList.add('choisie');l.appendChild(v);});
  $('nbImages').textContent=anim.length+' / '+etat.maxImages;
}
function ajouterImage(){if(anim.length>=etat.maxImages){message('16 images au maximum.');return;}
  anim.splice(choisie+1,0,enHex(px));choisie++;majImages();}
function remplacerImage(){if(choisie<0)return;anim[choisie]=enHex(px);majImages();}
function supprimerImage(){if(choisie<0)return;anim.splice(choisie,1);choisie=Math.min(choisie,anim.length-1);majImages();}
function majDelai(){$('delaiTexte').textContent=$('delai').value+' ms';}
function chargerAnim(a){anim=decouper(a.images);choisie=-1;$('delai').value=a.delai;majDelai();
  $('nomAnim').value=a.nom;majImages();jouerSans();}
function jouerSans(){if(anim.length)fetch('/animation?images='+anim.join('')+'&delai='+$('delai').value+'&nom='+encodeURIComponent($('nomAnim').value));}
function jouer(){if(!anim.length){message('Ajoute au moins une image.');return;}jouerSans();message('Animation lancée');}
function enregistrerAnim(){
  const nom=$('nomAnim').value.trim();
  if(!nom){message('Donne un nom à l\'animation.');return;}
  if(!anim.length){message('Ajoute au moins une image.');return;}
  requete('/animation/enregistrer?nom='+encodeURIComponent(nom)+'&images='+anim.join('')+'&delai='+$('delai').value,majListes);
}

// ----- Texte -----
function defiler(){
  const t=$('message_texte').value.trim();
  if(!t){message('Écris un message.');return;}
  requete('/texte?texte='+encodeURIComponent(t)+'&vitesse='+$('vitesse').value);
  message('Message envoyé');
}

// ----- État -----
function majListes(e){
  etat=e;
  const ld=$('listeDessins'),lf=$('listeFormes'),la=$('listeAnims'),lp=$('listePretes');
  [ld,lf,la,lp].forEach(l=>l.innerHTML='');
  e.dessins.forEach((d,i)=>ld.appendChild(vignette(d.nom,d.image,()=>{charger(d.image);$('nomDessin').value=d.nom;},
    ()=>requete('/supprimer?numero='+i,majListes))));
  if(!e.dessins.length)ld.textContent='Aucun dessin pour l\'instant.';
  e.formes.forEach(f=>lf.appendChild(vignette(f.nom,f.image,()=>charger(f.image))));
  e.animations.forEach((a,i)=>la.appendChild(vignette(a.nom,a.images.substr(0,16),()=>chargerAnim(a),
    ()=>requete('/animation/supprimer?numero='+i,majListes))));
  if(!e.animations.length)la.textContent='Aucune animation pour l\'instant.';
  e.animationsPretes.forEach(a=>lp.appendChild(vignette(a.nom,a.images.substr(0,16),()=>chargerAnim(a))));
  message(e.dessins.length+'/'+e.maxDessins+' dessins et '+e.animations.length+'/'+e.maxAnimations+' animations en mémoire');
}
fetch('/etat').then(r=>r.json()).then(e=>{
  px=depuisHex(e.image);dessiner();
  $('lum').value=e.lum;$('message_texte').value=e.texte;$('vitesse').value=e.vitesse;
  anim=decouper(e.anim.images);$('delai').value=e.anim.delai;$('nomAnim').value=e.anim.nom;majDelai();
  majListes(e);majImages();
  const onglet={dessin:'dessins',animation:'animation',texte:'texte'}[e.mode];
  document.querySelector('[data-p="'+onglet+'"]').click();
});
</script></body></html>)rawliteral";
