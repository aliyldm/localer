const { randomBytes, createHash } = require('node:crypto');

const ALPHABET = 'ABCDEFGHJKLMNPQRSTUVWXYZ23456789';
// Published by the official EasyTier/etcat project in relays.toml.
const DEFAULT_RELAY = 'tcp://38.76.179.190:11010';
const LEGACY_RELAYS = ['tcp://public.easytier.top:11010', 'tcp://public.easytier.cn:11010'];

function createCode() {
  // 26 independent base-32 symbols: 130 bits from the OS CSPRNG.
  return formatCode(Array.from(randomBytes(26), b => ALPHABET[b & 31]).join(''));
}

function formatCode(raw) {
  return `LCL-${raw.slice(0, 20).match(/.{1,5}/g).join('-')}-${raw.slice(20)}`;
}

function normalizeCode(input) {
  if (typeof input !== 'string' || input.length > 1024) throw new Error('Geçerli bir davet kodu gir.');
  let value = input.trim();
  if (/^localer:\/\//i.test(value)) value = new URL(value).pathname.replace(/^\//, '');
  const raw = value.toUpperCase().replace(/^LCL-?/, '').replace(/[\s-]/g, '');
  if (raw.length !== 26 || [...raw].some(c => !ALPHABET.includes(c))) {
    throw new Error('Davet kodu geçersiz. LCL ile başlayan kodun tamamını yapıştır.');
  }
  return formatCode(raw);
}

function validateRelay(input = DEFAULT_RELAY) {
  if (typeof input !== 'string' || input.length > 256) throw new Error('Buluşma sunucusu adresi geçersiz.');
  let url;
  try { url = new URL(input.trim()); } catch { throw new Error('Sunucu adresi tcp://adres:11010 biçiminde olmalı.'); }
  if (!['tcp:', 'udp:', 'ws:', 'wss:'].includes(url.protocol) || !url.hostname || url.username || url.password || url.search || url.hash || (url.pathname && url.pathname !== '/')) {
    throw new Error('Sunucu adresi tcp, udp, ws veya wss protokolü kullanmalı.');
  }
  if (!url.port && !['ws:', 'wss:'].includes(url.protocol)) throw new Error('Sunucu portunu belirt. Örnek: tcp://adres:11010');
  return url.toString().replace(/\/$/, '');
}

function parseInvite(input, fallbackRelay) {
  let relay = fallbackRelay || DEFAULT_RELAY;
  if (typeof input === 'string' && /^localer:\/\//i.test(input.trim())) {
    const url = new URL(input.trim());
    if (url.hostname.toLowerCase() !== 'join' || url.username || url.password || url.port || url.hash) throw new Error('Davet bağlantısı geçersiz.');
    relay = url.searchParams.get('relay') || DEFAULT_RELAY;
  }
  relay = validateRelay(relay);
  return { code: normalizeCode(input), relay: LEGACY_RELAYS.includes(relay) ? DEFAULT_RELAY : relay };
}

function credentials(code) {
  const normalized = normalizeCode(code);
  const hash = domain => createHash('sha256').update(`${domain}:${normalized}`).digest('hex');
  const networkHash = hash('localer-network-v1');
  return { name: `localer-${networkHash.slice(0, 24)}`, secret: hash('localer-secret-v1'), subnet: `10.${64 + parseInt(networkHash.slice(0, 2), 16) % 64}.${parseInt(networkHash.slice(2, 4), 16)}.1/24` };
}

function inviteLink(code, relay) {
  const url = new URL(`localer://join/${normalizeCode(code)}`);
  // Pin even the default address so future app versions join the same relay.
  url.searchParams.set('relay', validateRelay(relay));
  return url.toString();
}

module.exports = { createCode, normalizeCode, validateRelay, parseInvite, credentials, inviteLink, DEFAULT_RELAY };
