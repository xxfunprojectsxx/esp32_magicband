// Import CSS
import './index.css';

const API_BASE = location.hostname === '192.168.4.1' ? '' : 'http://192.168.4.1';

/* ---------- Request state management ---------- */
let isRequestInProgress = false;

function setButtonsDisabled(disabled) {
  const buttons = document.querySelectorAll('button');
  buttons.forEach(btn => {
    btn.disabled = disabled;
    if (disabled) {
      btn.style.opacity = '0.5';
      btn.style.cursor = 'not-allowed';
    } else {
      btn.style.opacity = '1';
      btn.style.cursor = 'pointer';
    }
  });
  isRequestInProgress = disabled;
}

/* ---------- small UI helpers ---------- */
function toast(msg, ok=true){
  const el = document.createElement('div');
  el.className='toast';
  el.textContent=msg;
  el.style.background = ok ? 'linear-gradient(135deg,#22c55e,#66d39a)' : 'linear-gradient(135deg,#ff6b6b,#ff8b6b)';
  el.style.color = '#021014';
  document.body.appendChild(el);
  setTimeout(()=>el.remove(), 1400);
}

/* palette mapping */
function hexToClosestPalette(hex){
  const colors = {
    'cyan': 0x00, 'purple': 0x01, 'blue': 0x02, 'brightpurple': 0x05,
    'pink': 0x08, 'yelloworange': 0x0F, 'lime': 0x12, 'orange': 0x13,
    'red': 0x15, 'green': 0x19, 'white': 0x1B
  };
  const r = parseInt(hex.substr(1,2),16);
  const g = parseInt(hex.substr(3,2),16);
  const b = parseInt(hex.substr(5,2),16);
  if(r>200 && g<100 && b<100) return colors.red;
  if(r<100 && g<100 && b>200) return colors.blue;
  if(r<100 && g>200 && b<100) return colors.green;
  if(r>200 && g>100 && b<100) return colors.orange;
  if(r>150 && g<100 && b>150) return colors.purple;
  if(r>200 && g>100 && b>150) return colors.pink;
  if(r<100 && g>200 && b>200) return colors.cyan;
  if(r>200 && g>200 && b<100) return colors.yelloworange;
  if(r>200 && g>200 && b>200) return colors.white;
  return colors.white;
}

/* send wrapper: wakes device then sends real command */
async function sendWithWakeup(body){
  if (isRequestInProgress) {
    toast('Request in progress...', false);
    return;
  }
  
  setButtonsDisabled(true);
  
  try {
    await fetch(`${API_BASE}/command`, { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'action=ping' }).catch(()=>{});
    await new Promise(r=>setTimeout(r,420));
    const res = await fetch(`${API_BASE}/command`, { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body });
    if(!res.ok) throw new Error('request failed');
    const text = await res.text();
    toast('Sent', true);
    return text;
  } catch(e){
    toast(`Error ${e}`, false);
    throw e;
  } finally {
    setButtonsDisabled(false);
  }
}

/* Presets */
window.sendPreset = function(color){
  const vibOn = document.getElementById('vibToggle').checked;
  const vib = vibOn ? document.getElementById('vibPattern').value : 0;
  const body = `action=preset&color=${encodeURIComponent(color)}&vib=${vib}`;
  sendWithWakeup(body).catch(()=>{});
}

/* Dual color */
window.sendDual = function(){
  const c1 = hexToClosestPalette(document.getElementById('dualInner').value);
  const c2 = hexToClosestPalette(document.getElementById('dualOuter').value);
  const vibOn = document.getElementById('vibToggle').checked;
  const vib = vibOn ? document.getElementById('vibPattern').value : 0;
  const body = `action=dual&c1=${c1}&c2=${c2}&vib=${vib}`;
  sendWithWakeup(body).catch(()=>{});
}

/* Crossfade */
window.sendCross = function(){
  const c1 = hexToClosestPalette(document.getElementById('crossA').value);
  const c2 = hexToClosestPalette(document.getElementById('crossB').value);
  const vibOn = document.getElementById('vibToggle').checked;
  const vib = vibOn ? document.getElementById('vibPattern').value : 0;
  const body = `action=crossfade&c1=${c1}&c2=${c2}&vib=${vib}`;
  sendWithWakeup(body).catch(()=>{});
}

/* Rainbow */
window.sendRainbow = function(){
  const c1 = hexToClosestPalette(document.getElementById('r1').value);
  const c2 = hexToClosestPalette(document.getElementById('r2').value);
  const c3 = hexToClosestPalette(document.getElementById('r3').value);
  const c4 = hexToClosestPalette(document.getElementById('r4').value);
  const c5 = hexToClosestPalette(document.getElementById('r5').value);
  const vibOn = document.getElementById('vibToggle').checked;
  const vib = vibOn ? document.getElementById('vibPattern').value : 0;
  const body = `action=rainbow&c1=${c1}&c2=${c2}&c3=${c3}&c4=${c4}&c5=${c5}&vib=${vib}`;
  sendWithWakeup(body).catch(()=>{});
}

/* Circle */
window.sendCircle = function(){
  const vibOn = document.getElementById('vibToggle').checked;
  const vib = vibOn ? document.getElementById('vibPattern').value : 0;
  const body = `action=circle&vib=${vib}`;
  sendWithWakeup(body).catch(()=>{});
}

/* Ping/wake */
window.sendPing = function(){
  if (isRequestInProgress) {
    toast('Request in progress...', false);
    return;
  }
  
  setButtonsDisabled(true);
  
  fetch(`${API_BASE}/command`, { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:'action=ping' })
    .then(r=>{ 
      if(r.ok) toast('Wake sent', true); 
      else toast('Wake failed', false); 
    })
    .catch(()=>toast('Wake failed', false))
    .finally(()=>setButtonsDisabled(false));
}

/* Manual */
window.sendManual = function(){
  const txt = document.getElementById('manual').value.trim();
  if(!txt){ toast('Enter command', false); return; }
  sendWithWakeup(txt).catch(()=>{});
}