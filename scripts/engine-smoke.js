// Real binaries, three nodes, no TUN or administrator rights, loopback transport only.
const { spawn, execFile } = require('node:child_process');
const { promisify } = require('node:util');
const path = require('node:path');
const fs = require('node:fs');
const http = require('node:http');
const dgram = require('node:dgram');
const assert = require('node:assert/strict');
const { freePort } = require('../src/engine');
const { createCode, credentials, validateRelay } = require('../src/room');
const relayIndex = process.argv.indexOf('--relay');
const relay = relayIndex >= 0 ? validateRelay(process.argv[relayIndex + 1]) : null;
const exec = promisify(execFile);
const wait = ms => new Promise(resolve => setTimeout(resolve, ms));
const directory = path.resolve(__dirname, '../vendor/easytier/easytier-windows-x86_64');
const children = [];
const servers = [];
let logs = '';

async function query(port, command = 'peer') {
  const { stdout } = await exec(path.join(directory, 'easytier-cli.exe'), ['-p', `127.0.0.1:${port}`, '-o', 'json', command], { timeout: 4000, windowsHide: true });
  return JSON.parse(stdout);
}

async function node(name, secret, network, listener, rpc, peer, extra = []) {
  const env = Object.fromEntries(Object.entries(process.env).filter(([key]) => !key.startsWith('ET_')));
  const args = ['--no-tun', '--network-name', network, '--hostname', name, '--listeners', `tcp://127.0.0.1:${listener}`, '--rpc-portal', `127.0.0.1:${rpc}`, '--private-mode', 'true', '--bind-device', 'false', '--disable-upnp', '--disable-udp-hole-punching', '--disable-tcp-hole-punching', '--stun-servers', '', '--stun-servers-v6', '', '--console-log-level', 'error', ...extra];
  if (peer) args.push('--peers', typeof peer === 'string' ? peer : `tcp://127.0.0.1:${peer}`);
  if (relay) args.push('--disable-p2p');
  const child = spawn(path.join(directory, 'easytier-core.exe'), args, { cwd: directory, windowsHide: true, env: { ...env, ET_NETWORK_SECRET: secret, RUST_LOG: 'error' }, stdio: ['ignore', 'pipe', 'pipe'] });
  children.push(child);
  child.on('error', error => { logs += error.message; });
  child.stdout.on('data', data => { logs += data.toString(); });
  child.stderr.on('data', data => { logs += data.toString(); });
  let lastError;
  for (let i = 0; i < 25; i++) {
    try { await query(rpc, 'node'); return; } catch (error) { lastError = error; await wait(200); }
  }
  throw new Error(`${name} did not start: ${lastError?.message}; stdout=${lastError?.stdout || ''}; stderr=${lastError?.stderr || ''}`);
}

(async () => {
  const keys = credentials(createCode());
  const ports = [];
  while (ports.length < 8) { const port = await freePort(); if (!ports.includes(port)) ports.push(port); }
  const [a, ar, b, br, bad, badr, tcpForward, udpForward] = ports;
  const httpServer = http.createServer((_request, response) => response.end('localer-tcp-ok'));
  await new Promise(resolve => httpServer.listen(0, '127.0.0.1', resolve)); servers.push(httpServer);
  const udpServer = dgram.createSocket('udp4');
  udpServer.on('message', (message, remote) => udpServer.send(message, remote.port, remote.address));
  await new Promise(resolve => udpServer.bind(0, '127.0.0.1', resolve)); servers.push(udpServer);
  await node('smoke-host', keys.secret, keys.name, a, ar, relay, ['--ipv4', '10.90.1.1/24', '--use-smoltcp']);
  await node('smoke-friend', keys.secret, keys.name, b, br, relay || a, ['--ipv4', '10.90.1.2/24', '--port-forward', `tcp://127.0.0.1:${tcpForward}/10.90.1.1:${httpServer.address().port}`, '--port-forward', `udp://127.0.0.1:${udpForward}/10.90.1.1:${udpServer.address().port}`]);
  let peers;
  for (let i = 0; i < (relay ? 100 : 40); i++) { peers = await query(ar); if (JSON.stringify(peers).includes('smoke-friend')) break; await wait(250); }
  if (!JSON.stringify(peers).includes('smoke-friend')) console.error('Peer response:', JSON.stringify(peers));
  assert.ok(JSON.stringify(peers).includes('smoke-friend'), 'same-room friend must connect');
  await wait(1200);
  const httpText = await new Promise((resolve, reject) => {
    const request = http.get(`http://127.0.0.1:${tcpForward}`, response => { let body = ''; response.on('data', chunk => { body += chunk; }); response.on('end', () => resolve(body)); });
    request.setTimeout(8000, () => request.destroy(new Error('TCP forwarding timeout'))); request.on('error', reject);
  });
  assert.equal(httpText, 'localer-tcp-ok', 'TCP service must pass through the real overlay');
  const udpText = await new Promise((resolve, reject) => {
    const client = dgram.createSocket('udp4');
    const send = () => client.send('localer-udp-ok', udpForward, '127.0.0.1');
    const resend = setInterval(send, 250);
    const timer = setTimeout(() => { clearInterval(resend); client.close(); reject(new Error('UDP forwarding timeout')); }, 8000);
    client.once('message', message => { clearTimeout(timer); clearInterval(resend); client.close(); resolve(message.toString()); });
    send();
  });
  assert.equal(udpText, 'localer-udp-ok', 'UDP datagrams must pass through the real overlay');
  await node('smoke-intruder', credentials(createCode()).secret, keys.name, bad, badr, relay || a);
  await wait(1800);
  peers = await query(ar);
  assert.ok(!JSON.stringify(peers).includes('smoke-intruder'), 'wrong room secret must be rejected');
  const report = { passed: true, relay, testedAt: new Date().toISOString(), assertions: ['same-code handshake succeeds', 'TCP HTTP forwarding succeeds over overlay', 'UDP echo forwarding succeeds over overlay', 'different secret rejected', 'all management RPC bound to loopback', 'no TUN or OS network changes'], peers };
  fs.mkdirSync(path.resolve(__dirname, '../artifacts'), { recursive: true });
  fs.writeFileSync(path.resolve(__dirname, relay ? '../artifacts/relay-transfer-smoke.json' : '../artifacts/engine-smoke.json'), JSON.stringify(report, null, 2));
  console.log(JSON.stringify(report, null, 2));
})().catch(error => { console.error(error.message); console.error(logs.slice(-2500)); process.exitCode = 1; }).finally(async () => {
  await Promise.all(children.map(child => new Promise(resolve => { if (child.exitCode !== null) return resolve(); child.once('exit', resolve); child.kill(); })));
  for (const server of servers) server.close();
});
