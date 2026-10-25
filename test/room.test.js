const { test } = require('node:test');
const assert = require('node:assert/strict');
const { createCode, normalizeCode, credentials, parseInvite, inviteLink, validateRelay, DEFAULT_RELAY } = require('../src/room');
const { buildArgs, normalizePeers } = require('../src/engine');

test('rooms have independent secrets and accept pasted, lower-case codes', () => {
  const codes = new Set(Array.from({ length: 1000 }, createCode));
  assert.equal(codes.size, 1000);
  const code = [...codes][0];
  assert.equal(normalizeCode(code.toLowerCase()), code);
  assert.deepEqual(credentials(code), credentials(code.toLowerCase()));
  assert.notEqual(credentials(code).secret, credentials([...codes][1]).secret);
  assert.match(credentials(code).subnet, /^10\.\d+\.\d+\.1\/24$/);
});
test('custom relay survives invitation round trip', () => {
  const code = createCode(); const relay = 'tcp://example.com:11010';
  assert.deepEqual(parseInvite(inviteLink(code, relay)), { code, relay });
  assert.deepEqual(parseInvite(inviteLink(code, DEFAULT_RELAY), relay), { code, relay: DEFAULT_RELAY });
});
test('invitations pin their relay and migrate removed legacy endpoints', () => {
  const code = createCode();
  assert.equal(new URL(inviteLink(code, DEFAULT_RELAY)).searchParams.get('relay'), DEFAULT_RELAY);
  assert.equal(parseInvite(`LOCALER://join/${code}?relay=tcp%3A%2F%2Fpublic.easytier.top%3A11010`).relay, DEFAULT_RELAY);
  assert.throws(() => parseInvite(`localer://user@join/${code}`));
});
test('malformed invites and executable relay strings are rejected', () => {
  for (const invalid of ['', 'LCL-1234', null, '<script>', 'I'.repeat(26)]) assert.throws(() => normalizeCode(invalid));
  for (const relay of ['https://example.com', 'tcp://user:secret@host:1', 'tcp://host:1/path', 'file:///C:/test', 'tcp://host:1?x=y']) assert.throws(() => validateRelay(relay));
  assert.throws(() => parseInvite(`localer://evil/${createCode()}`));
});
test('management binds only to loopback; secret never enters command line', () => {
  const room = { code: createCode(), relay: DEFAULT_RELAY };
  const args = buildArgs(room, 12345, 'Test');
  assert.ok(args.includes('127.0.0.1:12345'));
  assert.ok(args.includes('--private-mode'));
  assert.ok(!args.includes('--enable-exit-node'));
  assert.ok(!args.includes('--enable-udp-broadcast-relay'));
  assert.ok(!args.join(' ').includes(credentials(room.code).secret));
});
test('relay-only nodes are excluded from room participant list', () => {
  const peers = normalizePeers([{ ipv4: '10.1.1.1', hostname: 'Test', cost: 'Local', lat_ms: '*' }, { ipv4: '', hostname: 'Public relay', cost: 'p2p' }]);
  assert.equal(peers.length, 1); assert.equal(peers[0].local, true); assert.equal(peers[0].latency, null);
});
