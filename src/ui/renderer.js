const $ = selector => document.querySelector(selector);
let current = null, busy = false, toastTimer;

function showError(message) { $('#error').textContent = message || ''; $('#error').hidden = !message; }
function toast(message) { $('#toast').textContent = message; $('#toast').hidden = false; clearTimeout(toastTimer); toastTimer = setTimeout(() => { $('#toast').hidden = true; }, 2600); }
function setBusy(value) {
  busy = value;
  for (const selector of ['#create-room', '#join-room', '#elevate']) $(selector).disabled = value;
  $('#create-room').textContent = value ? 'Oda hazırlanıyor…' : 'Oda oluştur';
  $('#join-room').textContent = value ? 'Bağlanıyor…' : 'Odaya katıl';
}

for (const button of document.querySelectorAll('[data-tab]')) {
  button.addEventListener('click', () => {
    for (const tab of document.querySelectorAll('[data-tab]')) {
      const active = tab === button; tab.classList.toggle('active', active); tab.setAttribute('aria-selected', String(active)); tab.tabIndex = active ? 0 : -1;
      $(`#${tab.dataset.tab}-panel`).hidden = !active;
    }
    showError(null);
  });
  button.addEventListener('keydown', event => {
    if (!['ArrowLeft', 'ArrowRight', 'Home', 'End'].includes(event.key)) return;
    event.preventDefault();
    const tabs = [...document.querySelectorAll('[data-tab]')];
    const target = event.key === 'Home' ? tabs[0] : event.key === 'End' ? tabs.at(-1) : tabs[(tabs.indexOf(button) + 1) % tabs.length];
    target.click(); target.focus();
  });
}

async function perform(action) {
  if (busy) return;
  setBusy(true); showError(null);
  try { const response = await action(); if (!response.ok) showError(response.error); }
  catch { showError('Uygulamaya ulaşılamıyor. Localer’ı yeniden başlat.'); }
  finally { setBusy(false); }
}

$('#create-form').addEventListener('submit', event => {
  event.preventDefault(); perform(() => window.localer.create({ nickname: $('#nickname').value, relay: $('#relay').value }));
});
$('#join-form').addEventListener('submit', event => {
  event.preventDefault(); perform(() => window.localer.join({ invite: $('#invite').value, nickname: $('#join-nickname').value, relay: $('#relay').value }));
});
$('#leave').addEventListener('click', async () => {
  $('#leave').disabled = true; showError(null);
  try { const response = await window.localer.leave(); if (!response.ok) showError(response.error); }
  catch { showError('Bağlantı kapatılamadı. Uygulamayı yeniden başlat.'); }
  finally { $('#leave').disabled = false; }
});
$('#elevate').addEventListener('click', () => perform(() => window.localer.elevate()));
for (const [selector, value] of [['#copy-ip', () => current?.ip], ['#copy-code', () => current?.room?.code], ['#copy-invite', () => current?.room?.invite]]) {
  $(selector).addEventListener('click', async () => {
    const text = value(); if (!text) return;
    try { const response = await window.localer.copy(text); if (response.ok) toast('Panoya kopyalandı'); else showError(response.error); }
    catch { showError('Panoya kopyalanamadı. Tekrar dene.'); }
  });
}
$('#nickname').addEventListener('input', () => { $('#join-nickname').value = $('#nickname').value; });
$('#join-nickname').addEventListener('input', () => { $('#nickname').value = $('#join-nickname').value; });

function render(state) {
  current = state;
  const active = Boolean(state.room);
  $('#setup').hidden = active;
  $('#active-room').hidden = !active;
  $('#settings').hidden = active;
  const status = { idle: 'Bağlı değil', connecting: 'Bağlanıyor…', waiting: 'Arkadaşların bekleniyor', connected: 'Bağlandı', degraded: 'Sunucuya ulaşılamıyor', error: 'Bağlantı hatası' }[state.status] || 'Bağlı değil';
  $('#header-status').textContent = status;
  for (const selector of ['#header-dot', '#network-dot']) {
    $(selector).classList.toggle('online', ['waiting', 'connected'].includes(state.status));
    $(selector).classList.toggle('pending', ['connecting', 'degraded'].includes(state.status));
  }
  $('#network-status').textContent = status;
  $('#relay-status').textContent = state.message || (state.relayConnected ? 'Sunucu bağlantısı kuruldu. Davetini paylaşabilirsin.' : 'Sunucuya bağlanılıyor…');
  $('#my-ip').textContent = state.ip || 'Hazırlanıyor…';
  $('#copy-ip').disabled = !state.ip;
  $('#active-name').textContent = state.room?.name || 'Arkadaş odası';
  $('#room-code').textContent = state.room?.code || '';
  $('#invite-hint').textContent = state.room?.customRelay ? 'Özel sunucu kullanılıyor. Arkadaşlarına tam davet bağlantısını gönder.' : 'Bu kodu yalnızca güvendiğin kişilerle paylaş.';
  $('#copy-code').hidden = Boolean(state.room?.customRelay);
  const peers = state.peers || [];
  $('#peer-count').textContent = String(peers.length);
  $('#peers').replaceChildren();
  if (!peers.length) { const empty = document.createElement('p'); empty.className = 'empty-peers'; empty.textContent = 'Cihazlar aranıyor. Davet bağlantısını arkadaşlarınla paylaşabilirsin.'; $('#peers').append(empty); }
  for (const peer of peers) {
    const row = document.createElement('div'); row.className = 'peer-row';
    const avatar = document.createElement('span'); avatar.className = 'peer-avatar'; avatar.textContent = peer.name.slice(0, 2).toLocaleUpperCase('tr-TR');
    const info = document.createElement('div'); info.className = 'peer-info';
    const name = document.createElement('strong'); name.textContent = peer.name + (peer.local ? ' · Sen' : '');
    const ip = document.createElement('small'); ip.textContent = peer.ip;
    info.append(name, ip);
    const meta = document.createElement('div'); meta.className = 'peer-meta';
    const connection = document.createElement('div'); connection.textContent = peer.connection;
    const latency = document.createElement('div'); latency.textContent = peer.latency == null ? '—' : `${peer.latency} ms`;
    meta.append(connection, latency); row.append(avatar, info, meta); $('#peers').append(row);
  }
  $('#room-tip').textContent = state.ip ? `Örnek web adresi: http://${state.ip}:5173 · Yerel servisini 0.0.0.0 üzerinde dinlet ve gereken servis portuna güvenlik duvarında izin ver.` : 'Bağdaştırıcı hazırlanıyor. IP göründüğünde oyun veya servisine bu adres üzerinden bağlanabilirsiniz.';
  $('#log-output').textContent = state.logs?.length ? state.logs.map(line => `[${line.time}] ${line.message}`).join('\n') : 'Henüz bağlantı kaydı yok.';
  $('#logs').hidden = !(active || state.status === 'error' || state.logs?.length);
  if (state.status === 'error') showError(state.error);
  else showError(null);
}

window.localer.onState(render);
window.localer.info().then(response => {
  if (!response.ok) { showError(response.error); return; }
  const info = response.data;
  $('#nickname').value = info.nickname; $('#join-nickname').value = info.nickname;
  $('#relay').value = info.defaultRelay;
  $('#admin-notice').hidden = info.admin;
  $('#version').textContent = `LOCALER ${info.version}`;
  render(info.state);
}).catch(() => showError('Localer başlatılamadı. Uygulamayı yeniden aç.'));
