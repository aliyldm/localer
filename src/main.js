const { app, BrowserWindow, ipcMain, clipboard, dialog } = require('electron');
const { execFile } = require('node:child_process');
const { promisify } = require('node:util');
const path = require('node:path');
const fs = require('node:fs');
const os = require('node:os');
const { pathToFileURL } = require('node:url');
const { Engine } = require('./engine');
const { createCode, parseInvite, validateRelay, DEFAULT_RELAY } = require('./room');
const exec = promisify(execFile);
const smoke = process.argv.includes('--smoke-test');
let win, engine, isAdmin = false, quitting = false, operation = false;
const pageURL = pathToFileURL(path.join(__dirname, 'ui/index.html')).href;

if (!app.requestSingleInstanceLock()) app.quit();
else {
  app.on('second-instance', () => { if (win) { if (win.isMinimized()) win.restore(); win.show(); win.focus(); } });
  app.whenReady().then(start).catch(error => { if (smoke) console.error(error); else dialog.showErrorBox('Localer başlatılamadı', error.message); app.exit(1); });
}

function handle(channel, callback, mutation = false) {
  ipcMain.handle(channel, async (event, data) => {
    if (event.sender !== win?.webContents || event.senderFrame?.url !== pageURL) return { ok: false, error: 'Yetkisiz istek.' };
    if (mutation && operation) return { ok: false, error: 'Bir işlem devam ediyor, lütfen bekle.' };
    if (mutation) operation = true;
    try { return { ok: true, data: await callback(data) }; }
    catch (error) { return { ok: false, error: error.message }; }
    finally { if (mutation) operation = false; }
  });
}

function profile(data) {
  if (!data || typeof data !== 'object') throw new Error('Oda bilgileri eksik.');
  if (!isAdmin) throw new Error('Sanal ağ için yönetici yetkisi gerekiyor. “Yönetici olarak yeniden aç” düğmesini kullan.');
  const nickname = typeof data.nickname === 'string' ? data.nickname.trim() : '';
  if (!nickname || nickname.length > 32 || /[\x00-\x1f\x7f]/.test(nickname)) throw new Error('Cihaz adı 1–32 karakter olmalı.');
  return nickname;
}

async function start() {
  if (process.platform !== 'win32') throw new Error('Bu sürüm Windows x64 için hazırlandı.');
  try {
    const { stdout } = await exec('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', '([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)'], { windowsHide: true, timeout: 10000 });
    isAdmin = stdout.trim() === 'True';
  } catch { isAdmin = false; }
  engine = new Engine(app.isPackaged ? path.join(process.resourcesPath, 'easytier') : path.join(__dirname, '../vendor/easytier/easytier-windows-x86_64'));
  win = new BrowserWindow({ width: 800, height: 860, minWidth: 540, minHeight: 650, show: !smoke,
    backgroundColor: '#f6f7f9', title: 'Localer', icon: path.join(__dirname, '../assets/localer.ico'), autoHideMenuBar: true,
    webPreferences: { preload: path.join(__dirname, 'preload.js'), nodeIntegration: false, contextIsolation: true, sandbox: true, webSecurity: true, backgroundThrottling: !smoke }
  });
  win.webContents.setWindowOpenHandler(() => ({ action: 'deny' }));
  win.webContents.on('will-navigate', event => event.preventDefault());
  win.webContents.session.setPermissionRequestHandler((_contents, _permission, callback) => callback(false));
  win.webContents.on('render-process-gone', () => { engine.stop().finally(() => app.quit()); });
  engine.on('state', state => { if (!win.isDestroyed()) win.webContents.send('engine:state', state); });
  handle('app:info', () => ({ version: app.getVersion(), admin: isAdmin, nickname: os.hostname().slice(0, 32), defaultRelay: DEFAULT_RELAY, state: engine.snapshot() }));
  handle('room:create', async data => {
    const nickname = profile(data);
    const room = { code: createCode(), relay: validateRelay(data.relay || DEFAULT_RELAY), name: String(data.name || 'Arkadaş odası').trim().slice(0, 48), creator: true };
    return engine.start(room, nickname);
  }, true);
  handle('room:join', async data => {
    const nickname = profile(data);
    const room = { ...parseInvite(data.invite, data.relay), name: 'Arkadaş odası', creator: false };
    return engine.start(room, nickname);
  }, true);
  // Leaving must be able to cancel an in-flight connection attempt.
  handle('room:leave', () => engine.stop());
  handle('app:copy', value => {
    if (typeof value !== 'string' || value.length > 2048) throw new Error('Kopyalanacak metin geçersiz.');
    clipboard.writeText(value);
  });
  handle('app:elevate', async () => {
    if (isAdmin) return;
    const quote = value => "'" + value.replace(/'/g, "''") + "'";
    const args = app.isPackaged ? '' : ` -ArgumentList ${quote('"' + app.getAppPath() + '"')}`;
    // The elevated application is interactive and must open visibly.
    const command = `Start-Process -FilePath ${quote(process.execPath)}${args} -Verb RunAs -WindowStyle Normal`;
    // Release the application lock so the elevated copy can start.
    app.releaseSingleInstanceLock();
    try { await exec('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', command], { windowsHide: true, timeout: 120000 }); }
    catch { if (!app.requestSingleInstanceLock()) app.quit(); throw new Error('Yönetici isteği iptal edildi veya uygulama açılamadı.'); }
    app.quit();
  }, true);
  await win.loadFile(path.join(__dirname, 'ui/index.html'));
  if (smoke) {
    const uiErrors = [];
    win.webContents.on('console-message', (_event, details) => { if (details.level === 'error') uiErrors.push(details.message); });
    await new Promise(resolve => setTimeout(resolve, 700));
    const result = await win.webContents.executeJavaScript(`(async () => {
      const info = await window.localer.info();
      if (!info.ok || !document.querySelector('#create-room')) throw new Error('Arayüz veya IPC yüklenmedi');
      document.querySelector('[data-tab="join"]').click();
      if (document.querySelector('#join-panel').hidden) throw new Error('Katılma sekmesi çalışmıyor');
      document.querySelector('[data-tab="create"]').click();
      const createTab = document.querySelector('[data-tab="create"]');
      createTab.dispatchEvent(new KeyboardEvent('keydown', { key: 'ArrowRight', bubbles: true }));
      if (document.querySelector('[data-tab="join"]').getAttribute('aria-selected') !== 'true') throw new Error('Klavye ile sekme değiştirme başarısız');
      createTab.click();
      const invalid = await window.localer.join({ nickname: 'Smoke', invite: 'bad-code' });
      if (invalid.ok) throw new Error('Geçersiz katılım reddedilmedi');
      return { title: document.title, buttons: document.querySelectorAll('button').length, info: info.ok };
    })()`);
    const output = path.resolve(process.env.LOCALER_ARTIFACT_DIR || path.join(app.getAppPath(), 'artifacts')); fs.mkdirSync(output, { recursive: true });
    fs.writeFileSync(path.join(output, 'ui.png'), (await win.webContents.capturePage()).toPNG());
    await win.webContents.executeJavaScript(`render({ status: 'connected', room: { name: 'Arkadaş odası', code: 'LCL-ABCDE-FGHJK-LMNPQ-RSTUV-WXYZ23', invite: 'localer://join/test' }, ip: '10.90.1.1', relayConnected: true, peers: [{ name: '<script>Test</script>', ip: '10.90.1.1', local: true, connection: 'Bu cihaz', latency: null }, { name: 'Arkadaş-PC', ip: '10.90.1.2', local: false, connection: 'Doğrudan', latency: '12.4' }], logs: [] });`);
    const roomResult = await win.webContents.executeJavaScript(`({ visible: !document.querySelector('#active-room').hidden, rows: document.querySelectorAll('.peer-row').length, safeText: document.querySelector('.peer-info strong').textContent, scriptCount: document.querySelectorAll('#peers script').length })`);
    if (!roomResult.visible || roomResult.rows !== 2 || roomResult.scriptCount !== 0) throw new Error('Oda ekranı veya güvenli metin gösterimi başarısız');
    await new Promise(resolve => setTimeout(resolve, 350));
    fs.writeFileSync(path.join(output, 'room-ui-fixture.png'), (await win.webContents.capturePage()).toPNG());
    const stateChecks = await win.webContents.executeJavaScript(`(() => {
      const state = { status: 'degraded', room: { name: 'Test', code: 'Test', customRelay: true }, ip: '10.90.1.1', peers: [], logs: [], message: 'Sunucuya ulaşılamıyor.' };
      render(state);
      if (document.querySelector('#header-status').textContent !== 'Sunucuya ulaşılamıyor' || !document.querySelector('#copy-code').hidden) throw new Error('Bağlantı durumu veya özel sunucu daveti yanlış');
      render({ status: 'idle', room: null, peers: [], logs: [] });
      if (document.querySelector('#setup').hidden || !document.querySelector('#active-room').hidden) throw new Error('Ayrılma ekranı sıfırlanmadı');
      return { degraded: true, customRelayInvite: true, reset: true, theme: getComputedStyle(document.documentElement).colorScheme };
    })()`);
    fs.writeFileSync(path.join(output, 'ui-smoke.json'), JSON.stringify({ ...result, roomFixture: roomResult, stateChecks, uiErrors }, null, 2));
    if (uiErrors.length) throw new Error(uiErrors.join('\n'));
    console.log('UI smoke test passed:', JSON.stringify(result)); app.quit();
  }
}

app.on('window-all-closed', () => app.quit());
app.on('before-quit', event => {
  if (quitting || !engine) return;
  event.preventDefault(); quitting = true;
  engine.stop().finally(() => app.quit());
});
