const { test } = require('node:test');
const assert = require('node:assert/strict');
const { Engine, normalizePeers } = require('../src/engine');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { createCode, DEFAULT_RELAY } = require('../src/room');

const local = { hostname: 'Host', ipv4: '10.80.1.1', cost: 'Local', lat_ms: '-' };
const relay = { hostname: 'Relay', ipv4: '', cost: 'p2p' };
const friend = { hostname: 'Friend', ipv4: '10.80.1.2', cost: 'relay(2)', lat_ms: '40.5' };
function engineWith(rows, adapter = true) {
  const engine = new Engine('.', { networkInterfaces: () => adapter ? { Localer: [{ address: local.ipv4 }] } : {} });
  engine.child = {}; engine.startedAt = Date.now(); engine.query = async () => rows;
  return engine;
}

test('a running process without a real adapter is never connected', async () => {
  const engine = engineWith([local, relay, friend], false);
  await engine.poll(); assert.equal(engine.state.status, 'connecting'); assert.equal(engine.state.ip, null); assert.deepEqual(engine.state.peers, []);
});
test('adapter readiness alone does not claim successful network connectivity', async () => {
  const engine = engineWith([local]); engine.startedAt -= 16000;
  await engine.poll(); assert.equal(engine.state.status, 'degraded'); assert.equal(engine.state.relayConnected, false);
});
test('relay readiness waits for friends; actual friends make status connected', async () => {
  const engine = engineWith([local, relay]);
  await engine.poll(); assert.equal(engine.state.status, 'waiting'); assert.equal(engine.state.peers.length, 1);
  engine.query = async () => [local, relay, friend];
  await engine.poll(); assert.equal(engine.state.status, 'connected'); assert.equal(engine.state.peers.length, 2);
  assert.equal(engine.state.peers[1].connection, 'Aktarmalı'); assert.equal(engine.state.peers[1].latency, 40.5);
});
test('direct friends remain connected when the relay is unavailable', async () => {
  const engine = engineWith([local, { ...friend, cost: 'p2p' }]);
  await engine.poll(); assert.equal(engine.state.status, 'connected'); assert.equal(engine.state.relayConnected, false);
});
test('a stale query cannot overwrite a replacement session', async () => {
  const engine = engineWith([local]); let complete;
  engine.query = () => new Promise(resolve => { complete = resolve; });
  const polling = engine.poll(); engine.child = {}; complete([local, relay, friend]); await polling;
  assert.equal(engine.state.status, 'idle');
});
test('consecutive RPC failures stop the session and clear stale IPs', async () => {
  const engine = engineWith([local, relay]); await engine.poll();
  engine.query = async () => { throw new Error('No RPC'); };
  await engine.poll(); await engine.poll(); await engine.poll();
  assert.equal(engine.state.status, 'error'); assert.equal(engine.child, null); assert.equal(engine.state.ip, null); assert.deepEqual(engine.state.peers, []);
});
test('adapter timeout stops the motor instead of spinning forever', async () => {
  const engine = engineWith([local], false); engine.startedAt -= 36000;
  await engine.poll(); assert.equal(engine.state.status, 'error'); assert.equal(engine.child, null);
});
test('logs redact invitation credentials and handle wrapped peer responses', () => {
  const engine = engineWith([]); engine.secret = 'test-secret'; engine.state.room = { code: 'test-code' };
  engine.log('credentials test-secret test-code');
  assert.ok(!engine.logs[0].message.includes('test-secret')); assert.ok(!engine.logs[0].message.includes('test-code'));
  assert.equal(normalizePeers({ peer_infos: [local, relay] }).length, 1);
});
test('disconnect cancels startup before any process is spawned', async () => {
  const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'localer-cancel-'));
  try {
    for (const file of ['easytier-core.exe', 'easytier-cli.exe', 'wintun.dll']) fs.writeFileSync(path.join(directory, file), '');
    const engine = new Engine(directory);
    const starting = engine.start({ code: createCode(), relay: DEFAULT_RELAY }, 'Test');
    await engine.stop(); await starting;
    assert.equal(engine.state.status, 'idle'); assert.equal(engine.child, null); assert.equal(engine.state.room, null);
  } finally {
    for (const file of ['easytier-core.exe', 'easytier-cli.exe', 'wintun.dll']) fs.unlinkSync(path.join(directory, file));
    fs.rmdirSync(directory);
  }
});
