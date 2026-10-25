// Must run elevated. One real Windows TUN + one rootless peer, via the public relay.
const { Engine, freePort, buildArgs } = require('../src/engine');
const { createCode, credentials, DEFAULT_RELAY } = require('../src/room');
const { spawn, execFile } = require('node:child_process');
const { promisify } = require('node:util');
const http = require('node:http');
const dgram = require('node:dgram');
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const assert = require('node:assert/strict');
const { randomUUID } = require('node:crypto');
const exec = promisify(execFile), wait = ms => new Promise(resolve => setTimeout(resolve, ms));
const root = path.resolve(__dirname, '..');
const directory = path.join(root, 'vendor/easytier/easytier-windows-x86_64');
const engine = new Engine(directory);
let friend, tcpServer, udpServer, hostIp, friendIp;
let friendLogs = '', requestCount = 0;
const allowTestPorts = process.argv.includes('--allow-test-ports');
const firewallNames = [`Localer-Test-${randomUUID()}-TCP`, `Localer-Test-${randomUUID()}-UDP`];
const psQuote = value => "'" + String(value).replace(/'/g, "''") + "'";
const report = { passed: false, startedAt: new Date().toISOString(), assertions: [] };

async function getHttp(port, address = '127.0.0.1') {
  return new Promise((resolve, reject) => {
    const request = http.get({ hostname: address, port, agent: false }, response => {
      let body = ''; response.on('data', chunk => { body += chunk; });
      response.on('end', () => { clearTimeout(timer); resolve(body); });
      response.on('error', error => { clearTimeout(timer); reject(error); });
    });
    const timer = setTimeout(() => request.destroy(new Error('TCP test timeout')), 12000);
    request.on('error', error => { clearTimeout(timer); reject(error); });
  });
}

async function getUdp(port) {
  return new Promise((resolve, reject) => {
    const client = dgram.createSocket('udp4');
    const send = () => client.send('real-tun-udp-ok', port, '127.0.0.1');
    const retry = setInterval(send, 300), timeout = setTimeout(() => { clearInterval(retry); client.close(); reject(new Error('UDP test timeout')); }, 6000);
    client.once('message', message => { clearInterval(retry); clearTimeout(timeout); client.close(); resolve(message.toString()); }); send();
  });
}

async function transferChecks(tcpPort, udpPort) {
  const result = {};
  try { result.tcp = await getHttp(tcpPort) === 'real-tun-tcp-ok'; } catch (error) { result.tcp = false; result.tcpError = error.message; }
  try { result.udp = await getUdp(udpPort) === 'real-tun-udp-ok'; } catch (error) { result.udp = false; result.udpError = error.message; }
  return result;
}

(async () => {
  const { stdout } = await exec('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', '([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)'], { windowsHide: true });
  assert.equal(stdout.trim(), 'True', 'Windows administrator token required');
  const room = { code: createCode(), relay: DEFAULT_RELAY, name: 'Geçici test odası' };
  await engine.start(room, 'localer-tun-test');
  for (let i = 0; i < 45 && !engine.state.ip; i++) { await wait(800); await engine.poll(); }
  assert.ok(engine.state.ip, 'real Windows TUN must receive virtual IP');
  report.assertions.push('real Windows virtual adapter created with DHCP IP');
  hostIp = engine.state.ip;
  tcpServer = http.createServer((_request, response) => { requestCount++; response.end('real-tun-tcp-ok'); });
  await new Promise((resolve, reject) => { tcpServer.once('error', reject); tcpServer.listen(0, hostIp, resolve); });
  udpServer = dgram.createSocket('udp4');
  udpServer.on('message', (message, remote) => udpServer.send(message, remote.port, remote.address));
  await new Promise((resolve, reject) => { udpServer.once('error', reject); udpServer.bind(0, hostIp, resolve); });
  assert.equal(await getHttp(tcpServer.address().port, hostIp), 'real-tun-tcp-ok', 'service must accept direct local requests');
  report.assertions.push('HTTP service responds locally on actual virtual IP');
  const rpc = await freePort(), forwardTcp = await freePort(), forwardUdp = await freePort();
  const args = buildArgs(room, rpc, 'localer-rootless-friend').filter(arg => arg !== '--dhcp');
  friendIp = hostIp.split('.').slice(0, 3).join('.') + '.250';
  args[args.indexOf('--ipv4') + 1] = friendIp + '/24';
  args.push('--no-tun', '--disable-p2p', '--port-forward', `tcp://127.0.0.1:${forwardTcp}/${hostIp}:${tcpServer.address().port}`, '--port-forward', `udp://127.0.0.1:${forwardUdp}/${hostIp}:${udpServer.address().port}`);
  const env = Object.fromEntries(Object.entries(process.env).filter(([key]) => !key.toUpperCase().startsWith('ET_')));
  friend = spawn(path.join(directory, 'easytier-core.exe'), args, { cwd: directory, windowsHide: true, env: { ...env, ET_NETWORK_SECRET: credentials(room.code).secret, RUST_LOG: 'error' }, stdio: ['ignore', 'pipe', 'pipe'] });
  for (const stream of [friend.stdout, friend.stderr]) stream.on('data', chunk => { friendLogs = (friendLogs + chunk.toString().split(credentials(room.code).secret).join('[redacted]')).slice(-6000); });
  for (let i = 0; i < 50 && !engine.state.peers.some(peer => !peer.local); i++) { await wait(500); await engine.poll(); }
  assert.ok(engine.state.peers.some(peer => peer.name === 'localer-rootless-friend'), 'friend must appear in production engine state');
  assert.equal(engine.state.status, 'connected');
  report.assertions.push('production engine reports friend over public relay');
  report.baseline = await transferChecks(forwardTcp, forwardUdp);
  let transfers = report.baseline;
  if ((!transfers.tcp || !transfers.udp) && allowTestPorts) {
    // Only this Node executable, test adapter, test virtual IPs and ephemeral service ports.
    const common = `-Direction Inbound -Action Allow -Program ${psQuote(process.execPath)} -InterfaceAlias ${psQuote(`Localer-${engine.port}`)} -LocalAddress ${psQuote(hostIp)} -RemoteAddress ${psQuote(friendIp)} -Profile Any`;
    await exec('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', `$ErrorActionPreference = 'Stop'; New-NetFirewallRule -Name ${psQuote(firewallNames[0])} -DisplayName 'Localer temporary TCP test' ${common} -Protocol TCP -LocalPort ${tcpServer.address().port} | Out-Null; New-NetFirewallRule -Name ${psQuote(firewallNames[1])} -DisplayName 'Localer temporary UDP test' ${common} -Protocol UDP -LocalPort ${udpServer.address().port} | Out-Null`], { windowsHide: true, timeout: 20000 });
    report.usedTemporaryFirewallRules = true;
    transfers = report.withScopedFirewallRules = await transferChecks(forwardTcp, forwardUdp);
  }
  const { stdout: statistics } = await exec('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', `Get-NetAdapterStatistics -Name ${psQuote(`Localer-${engine.port}`)} | Select-Object ReceivedBytes,SentBytes,ReceivedUnicastPackets,SentUnicastPackets | ConvertTo-Json -Compress`], { windowsHide: true, timeout: 10000 });
  report.adapterStatistics = JSON.parse(statistics); report.httpRequestsReceived = requestCount;
  assert.ok(transfers.tcp, 'TCP must cross actual Windows TUN'); report.assertions.push('TCP crosses public relay and real Windows TUN');
  assert.ok(transfers.udp, 'UDP must cross actual Windows TUN'); report.assertions.push('UDP crosses public relay and real Windows TUN');
  tcpServer.close(); tcpServer = null; udpServer.close(); udpServer = null;
  await engine.stop();
  for (let i = 0; i < 15 && Object.values(os.networkInterfaces()).flat().some(address => address?.address === hostIp); i++) await wait(300);
  assert.ok(!Object.values(os.networkInterfaces()).flat().some(address => address?.address === hostIp), 'virtual IP must be removed after disconnect');
  report.assertions.push('disconnect removes virtual IP');
  report.passed = true;
})().catch(error => { report.error = error.message; process.exitCode = 1; }).finally(async () => {
  tcpServer?.close(); udpServer?.close();
  if (friend && friend.exitCode === null && !friend.signalCode) await new Promise(resolve => { friend.once('exit', resolve); friend.kill(); });
  await engine.stop();
  if (allowTestPorts) {
    try {
      const names = firewallNames.map(psQuote).join(',');
      const { stdout } = await exec('powershell.exe', ['-NoProfile', '-NonInteractive', '-Command', `Get-NetFirewallRule -Name ${names} -ErrorAction SilentlyContinue | Remove-NetFirewallRule -ErrorAction Stop; @(Get-NetFirewallRule -Name ${names} -ErrorAction SilentlyContinue).Count`], { windowsHide: true, timeout: 20000 });
      report.firewallRulesRemoved = stdout.trim() === '0';
      if (!report.firewallRulesRemoved) { report.passed = false; process.exitCode = 1; }
    } catch (error) { report.firewallCleanupError = error.message; report.passed = false; process.exitCode = 1; }
  }
  if (hostIp) report.virtualIpRemoved = !Object.values(os.networkInterfaces()).flat().some(address => address?.address === hostIp);
  report.friendLogs = friendLogs; report.logs = engine.logs; report.finishedAt = new Date().toISOString();
  fs.mkdirSync(path.join(root, 'artifacts'), { recursive: true });
  fs.writeFileSync(path.join(root, 'artifacts/tun-smoke.json'), JSON.stringify(report, null, 2));
  console.log(JSON.stringify(report, null, 2));
});
