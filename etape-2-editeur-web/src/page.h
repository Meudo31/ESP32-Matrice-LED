// Page web de l'éditeur, servie par l'ESP32 sur http://matrice.local
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
.outils{display:flex;flex-wrap:wrap;gap:6px;margin-top:10px}
button{background:#2c3038;color:#eee;border:0;border-radius:8px;padding:10px 12px;font-size:1em}
button:active{background:#454b57}
input[type=text]{flex:1;min-width:0;background:#2c3038;color:#eee;border:0;border-radius:8px;padding:10px;font-size:1em}
input[type=range]{width:100%}
.liste{display:grid;grid-template-columns:repeat(auto-fill,minmax(70px,1fr));gap:8px}
.vignette{background:#22252b;border-radius:8px;padding:6px;text-align:center;font-size:.75em;position:relative}
.mini{display:grid;grid-template-columns:repeat(8,1fr);gap:1px;margin-bottom:4px}
.mini i{aspect-ratio:1;background:#333}
.mini i.on{background:#ff3030}
.suppr{position:absolute;top:-6px;right:-6px;padding:2px 7px;border-radius:50%;background:#7a2020;font-size:.9em}
#message{min-height:1.2em;color:#ffb347;margin-top:8px}
</style></head><body>
<h1>Matrice LED 8x8</h1>
<div id="grille"></div>
<div class="outils">
<button onclick="outil('effacer')">Effacer</button>
<button onclick="outil('remplir')">Remplir</button>
<button onclick="outil('inverser')">Inverser</button>
<button onclick="outil('tourner')">Tourner</button>
<button onclick="outil('miroir')">Miroir</button>
</div>
<div class="outils">
<button onclick="decaler(-1,0)">&larr;</button>
<button onclick="decaler(1,0)">&rarr;</button>
<button onclick="decaler(0,-1)">&uarr;</button>
<button onclick="decaler(0,1)">&darr;</button>
</div>
<h2>Luminosité</h2>
<input type="range" id="lum" min="0" max="15" oninput="luminosite(this.value)">
<h2>Enregistrer ce dessin</h2>
<div class="outils"><input type="text" id="nom" maxlength="15" placeholder="Nom du dessin"><button onclick="enregistrer()">Enregistrer</button></div>
<div id="message"></div>
<h2>Mes dessins</h2>
<div class="liste" id="dessins"></div>
<h2>Formes prêtes</h2>
<div class="liste" id="formes"></div>
<script>
let px=new Array(64).fill(0), crayon=null, attente=null;
const grille=document.getElementById('grille');
for(let i=0;i<64;i++){const d=document.createElement('div');d.dataset.i=i;grille.appendChild(d);}

// Dessin au doigt ou à la souris : le premier point décide si on allume ou si on éteint
function caseSous(e){const el=document.elementFromPoint(e.clientX,e.clientY);return el&&el.parentNode===grille?+el.dataset.i:-1;}
grille.addEventListener('pointerdown',e=>{const i=caseSous(e);if(i<0)return;crayon=px[i]?0:1;poser(i);});
grille.addEventListener('pointermove',e=>{if(crayon===null)return;const i=caseSous(e);if(i>=0)poser(i);});
addEventListener('pointerup',()=>crayon=null);
function poser(i){if(px[i]===crayon)return;px[i]=crayon;dessiner();envoyer();}

function dessiner(){grille.childNodes.forEach((d,i)=>d.classList.toggle('on',!!px[i]));}
function enHex(p){let s='';for(let y=0;y<8;y++){let o=0;for(let x=0;x<8;x++)if(p[y*8+x])o|=128>>x;s+=o.toString(16).padStart(2,'0');}return s.toUpperCase();}
function depuisHex(h){const p=[];for(let y=0;y<8;y++){const o=parseInt(h.substr(y*2,2),16);for(let x=0;x<8;x++)p.push((o>>(7-x))&1);}return p;}

// Envoi groupé : au plus un envoi toutes les 60 ms pendant qu'on dessine
function envoyer(){if(attente)return;attente=setTimeout(()=>{attente=null;fetch('/afficher?image='+enHex(px));},60);}

function outil(nom){
  if(nom=='effacer')px.fill(0);
  if(nom=='remplir')px.fill(1);
  if(nom=='inverser')px=px.map(v=>1-v);
  if(nom=='tourner'){const n=[];for(let y=0;y<8;y++)for(let x=0;x<8;x++)n.push(px[(7-x)*8+y]);px=n;}
  if(nom=='miroir'){const n=[];for(let y=0;y<8;y++)for(let x=0;x<8;x++)n.push(px[y*8+7-x]);px=n;}
  dessiner();envoyer();
}
function decaler(dx,dy){const n=[];for(let y=0;y<8;y++)for(let x=0;x<8;x++)n.push(px[((y-dy+8)%8)*8+(x-dx+8)%8]);px=n;dessiner();envoyer();}
function luminosite(v){fetch('/luminosite?valeur='+v);}
function message(t){document.getElementById('message').textContent=t;}

function vignette(item,numero){
  const v=document.createElement('div');v.className='vignette';
  const m=document.createElement('div');m.className='mini';
  depuisHex(item.image).forEach(b=>{const c=document.createElement('i');if(b)c.className='on';m.appendChild(c);});
  v.appendChild(m);v.appendChild(document.createTextNode(item.nom));
  v.onclick=()=>{px=depuisHex(item.image);dessiner();envoyer();document.getElementById('nom').value=numero>=0?item.nom:'';};
  if(numero>=0){const s=document.createElement('button');s.className='suppr';s.textContent='×';
    s.onclick=e=>{e.stopPropagation();if(confirm('Supprimer « '+item.nom+' » ?'))fetch('/supprimer?numero='+numero).then(r=>r.json()).then(majListes);};
    v.appendChild(s);}
  return v;
}
function majListes(etat){
  const d=document.getElementById('dessins'),f=document.getElementById('formes');
  d.innerHTML='';f.innerHTML='';
  etat.dessins.forEach((x,i)=>d.appendChild(vignette(x,i)));
  if(!etat.dessins.length)d.textContent='Aucun dessin pour l\'instant.';
  etat.formes.forEach(x=>f.appendChild(vignette(x,-1)));
  message(etat.dessins.length+' / '+etat.max+' dessins en mémoire');
}
function enregistrer(){
  const nom=document.getElementById('nom').value.trim();
  if(!nom){message('Donne un nom au dessin.');return;}
  fetch('/enregistrer?nom='+encodeURIComponent(nom)+'&image='+enHex(px))
    .then(r=>r.ok?r.json().then(majListes):r.text().then(message));
}
fetch('/etat').then(r=>r.json()).then(etat=>{
  px=depuisHex(etat.image);dessiner();
  document.getElementById('lum').value=etat.lum;
  majListes(etat);
});
</script></body></html>)rawliteral";
