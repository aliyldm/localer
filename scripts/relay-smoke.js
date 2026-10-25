const { spawn, execFile } = require('node:child_process');
const { promisify } = require('node:util');
const path = require('node:path');
const fs = require('node:fs');
const { freePort, buildArgs } = require('../src/engine');
const { createCode, credentials, DEFAULT_RELAY, validateRelay } = require('../src/room');
const exec = promisify(execFile);
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));

(async () => {
  const directory = path.resolve(__dirname, '../vendor/easytier/easytier-windows-x86_64');
  const relay = validateRelay(process.argv[2] || DEFAULT_RELAY);
  const room = { code: createCode(), relay };
  const rpc = await freePort(); const keys = credentials(room.code);
  const args = [...buildArgs(room, rpc, 'localer-relay-test'), '--no-tun'];
  const env = Object.fromEntries(Object.entries(process.env).filter(([key]) => !key.startsWith('ET_')));
  const child = spawn(path.join(directory, 'easytier-core.exe'), args, { cwd: directory, env: { ...env, ET_NETWORK_SECRET: keys.secret }, windowsHide: true, stdio: 'ignore' });
  try {
    let connected = false;
    for (let i = 0; i < 25; i++) {
      await sleep(1000);
      try {
        const { stdout } = await exec(path.join(directory, 'easytier-cli.exe'), ['-p', `127.0.0.1:${rpc}`, '-o', 'json', 'peer'], { windowsHide: true, timeout: 3000 });
        connected = JSON.parse(stdout).some(row => row.cost !== 'Local');
        if (connected) break;
      } catch { /* startup */ }
    }
    const result = { relay, connected, testedAt: new Date().toISOString(), tunCreated: false };
    fs.mkdirSync(path.resolve(__dirname, '../artifacts'), { recursive: true });
    fs.writeFileSync(path.resolve(__dirname, '../artifacts/relay-smoke.json'), JSON.stringify(result, null, 2));
    console.log(JSON.stringify(result));
    if (!connected) process.exitCode = 1;
  } finally {
    if (child.exitCode === null) await new Promise(resolve => { child.once('exit', resolve); child.kill(); });
  }
})().catch(error => { console.error(error.message); process.exitCode = 1; });
