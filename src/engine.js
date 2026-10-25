const { EventEmitter } = require('node:events');
const { spawn, execFile } = require('node:child_process');
const { promisify } = require('node:util');
const net = require('node:net');
const path = require('node:path');
const os = require('node:os');
const fs = require('node:fs');
const { credentials, inviteLink, DEFAULT_RELAY } = require('./room');
const exec = promisify(execFile);
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
const idleState = () => ({ status: 'idle', room: null, ip: null, peers: [], error: null, message: '', relayConnected: false });

async function freePort() {
  const server = net.createServer();
  await new Promise((resolve, reject) => { server.once('error', reject); server.listen(0, '127.0.0.1', resolve); });
  const port = server.address().port;
  await new Promise(resolve => server.close(resolve));
  return port;
}

function buildArgs(room, rpcPort, nickname) {
  const keys = credentials(room.code);
  return ['--network-name', keys.name, '--ipv4', keys.subnet, '--dhcp',
    '--hostname', nickname, '--dev-name', `Localer-${rpcPort}`, '--peers', room.relay,
    '--listeners', 'tcp://0.0.0.0:0', 'udp://0.0.0.0:0',
    '--rpc-portal', `127.0.0.1:${rpcPort}`, '--rpc-portal-whitelist', '127.0.0.1/32',
    '--private-mode', 'true', '--disable-upnp', '--bind-device', 'false', '--accept-dns', 'false',
    '--console-log-level', 'warn', '--file-log-level', 'off'];
}

function cleanOutput(text) { return text.replace(/\x1b\[[0-9;]*m/g, '').trim(); }
function peerRows(data) { return Array.isArray(data) ? data : (data?.peers || data?.peer_infos || []); }
function normalizePeers(data) {
  return peerRows(data).filter(row => net.isIPv4(String(row.ipv4).split('/')[0])).map(row => ({
    name: String(row.hostname || 'İsimsiz cihaz').slice(0, 64), ip: String(row.ipv4).split('/')[0],
    local: String(row.cost).toLowerCase() === 'local',
    connection: String(row.cost).toLowerCase() === 'local' ? 'Bu cihaz' : String(row.cost).toLowerCase() === 'p2p' ? 'Doğrudan' : 'Aktarmalı',
    latency: Number.isFinite(Number(row.lat_ms)) && row.lat_ms !== '' && row.lat_ms != null ? Number(row.lat_ms) : null,
    protocol: String(row.tunnel_proto || '')
  }));
}

class Engine extends EventEmitter {
  constructor(directory, options = {}) {
    super(); this.directory = directory; this.child = null; this.timer = null; this.logs = [];
    this.generation = 0; this.queryFailures = 0;
    this.networkInterfaces = options.networkInterfaces || os.networkInterfaces;
    this.state = idleState();
  }
  snapshot() { return { ...this.state, logs: [...this.logs] }; }
  update(patch) { Object.assign(this.state, patch); this.emit('state', this.snapshot()); }
  log(value) {
    const line = cleanOutput(String(value));
    if (!line) return;
    let safe = line;
    for (const secret of [this.secret, this.state.room?.code]) if (secret) safe = safe.split(secret).join('[gizli]');
    this.logs.push({ time: new Date().toLocaleTimeString('tr-TR', { timeZone: 'Europe/Istanbul', hour12: false }), message: safe.slice(0, 1500) });
    this.logs = this.logs.slice(-100);
  }
  async query(command) {
    const { stdout } = await exec(path.join(this.directory, 'easytier-cli.exe'), ['-p', `127.0.0.1:${this.port}`, '-o', 'json', ...command], {
      windowsHide: true, timeout: 4000, maxBuffer: 2 * 1024 * 1024, cwd: this.directory
    });
    return JSON.parse(cleanOutput(stdout));
  }
  async start(room, nickname) {
    if (this.child) throw new Error('Önce mevcut odadan ayrıl.');
    for (const file of ['easytier-core.exe', 'easytier-cli.exe', 'wintun.dll']) {
      if (!fs.existsSync(path.join(this.directory, file))) throw new Error(`Ağ motoru eksik: ${file}. Uygulamayı yeniden indir.`);
    }
    const generation = ++this.generation;
    const port = await freePort();
    if (generation !== this.generation) return this.snapshot();
    this.logs = []; this.port = port; this.secret = credentials(room.code).secret;
    this.startedAt = Date.now(); this.queryFailures = 0;
    this.update({ ...idleState(), status: 'connecting', message: 'Sanal ağ hazırlanıyor…', room: { ...room, customRelay: room.relay !== DEFAULT_RELAY, invite: inviteLink(room.code, room.relay) } });
    this.log('Sanal ağ motoru başlatılıyor.');
    const env = Object.fromEntries(Object.entries(process.env).filter(([key]) => !key.toUpperCase().startsWith('ET_')));
    this.child = spawn(path.join(this.directory, 'easytier-core.exe'), buildArgs(room, port, nickname), {
      cwd: this.directory, env: { ...env, ET_NETWORK_SECRET: this.secret, RUST_LOG: 'warn' }, windowsHide: true, stdio: ['ignore', 'pipe', 'pipe']
    });
    const child = this.child;
    for (const stream of [child.stdout, child.stderr]) stream.on('data', chunk => { if (this.child === child) this.log(chunk.toString()); });
    child.on('error', error => { if (this.child === child) { this.log(error.message); this.update({ status: 'error', error: 'Ağ motoru başlatılamadı.', ip: null, peers: [], relayConnected: false }); } });
    child.on('exit', code => {
      if (this.child !== child) return;
      this.child = null; clearTimeout(this.timer); this.secret = null;
      this.log(`Ağ motoru durdu (${code}).`);
      this.update({ status: 'error', ip: null, peers: [], relayConnected: false, error: 'Ağ motoru beklenmedik şekilde kapandı. Odadan ayrılıp tekrar bağlan.' });
    });
    try {
      for (let attempt = 0; attempt < 30; attempt++) {
        if (generation !== this.generation) return this.snapshot();
        if (this.child !== child || child.exitCode !== null) throw new Error(this.state.error || 'Ağ motoru durdu.');
        try {
          await this.query(['node']);
          if (generation !== this.generation) return this.snapshot();
          await this.poll();
          if (generation === this.generation) this.schedulePoll();
          return this.snapshot();
        } catch (error) { if (attempt === 29) throw error; }
        await sleep(300);
      }
    } catch {
      if (generation !== this.generation) return this.snapshot();
      await this.stop();
      this.update({ status: 'error', error: 'Sanal ağ başlatılamadı. Yönetici iznini ve bağlantı günlüğünü kontrol et.' });
      throw new Error(this.state.error);
    }
  }
  schedulePoll() {
    if (!this.child) return;
    clearTimeout(this.timer);
    const activeChild = this.child;
    this.timer = setTimeout(async () => { await this.poll(); if (this.child === activeChild) this.schedulePoll(); }, 2500);
  }
  async poll() {
    const activeChild = this.child;
    try {
      const rows = peerRows(await this.query(['peer']));
      if (!activeChild || this.child !== activeChild) return;
      this.queryFailures = 0;
      const peers = normalizePeers(rows), local = peers.find(peer => peer.local);
      const addresses = Object.values(this.networkInterfaces()).flat().filter(Boolean);
      const ip = local && addresses.some(address => address.address === local.ip) ? local.ip : null;
      const elapsed = Date.now() - this.startedAt;
      if (!ip && elapsed > 35000) {
        await this.stop();
        this.update({ status: 'error', error: 'Sanal ağ bağdaştırıcısı oluşturulamadı. Yönetici iznini ve bağlantı günlüğünü kontrol et.' });
        return;
      }
      const remoteCount = peers.filter(peer => !peer.local).length;
      const relayConnected = rows.some(row => !net.isIPv4(String(row.ipv4).split('/')[0]) && String(row.cost).toLowerCase() === 'p2p');
      let status = 'connecting', message = ip ? 'Sanal IP hazır; sunucuya bağlanılıyor…' : 'Sanal ağ bağdaştırıcısı hazırlanıyor…';
      if (ip && remoteCount) { status = 'connected'; message = `${remoteCount} arkadaş cihazına bağlantı var.`; }
      else if (ip && relayConnected) { status = 'waiting'; message = 'Oda hazır. Davet kodunu arkadaşlarınla paylaş.'; }
      else if (ip && elapsed > 15000) { status = 'degraded'; message = 'Sunucuya ulaşılamıyor. Bağlantı otomatik olarak yeniden deneniyor. Gerekirse odadan ayrılıp sunucu adresini değiştir.'; }
      this.update({ status, ip, peers: ip ? peers : [], relayConnected, message, error: null });
    } catch {
      if (this.child !== activeChild || !activeChild) return;
      this.queryFailures++;
      if (this.queryFailures >= 3) {
        this.log('Ağ motoru üç ardışık durum sorgusuna yanıt vermedi.');
        await this.stop();
        this.update({ status: 'error', error: 'Ağ motoru yanıt vermiyor. Yeniden bağlanmayı dene.' });
      } else this.update({ status: 'connecting', ip: null, peers: [], relayConnected: false, message: 'Ağ motorundan yanıt bekleniyor…', error: null });
    }
  }
  async stop() {
    const generation = ++this.generation;
    clearTimeout(this.timer);
    const child = this.child; this.child = null;
    if (child?.pid && child.exitCode === null && !child.signalCode) {
      await new Promise(resolve => {
        const timer = setTimeout(resolve, 3000);
        child.once('exit', () => { clearTimeout(timer); resolve(); });
        child.kill();
      });
      if (child.exitCode === null && !child.signalCode) {
        await exec('taskkill.exe', ['/PID', String(child.pid), '/T', '/F'], { windowsHide: true, timeout: 4000 }).catch(() => {});
      }
    }
    if (this.generation !== generation) return;
    this.secret = null; this.queryFailures = 0;
    this.update(idleState());
  }
}

module.exports = { Engine, freePort, buildArgs, normalizePeers };