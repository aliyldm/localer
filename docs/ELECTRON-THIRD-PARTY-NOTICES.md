# Third-party components

Localer is an independent desktop interface. It is not an official EasyTier product.

## EasyTier 2.6.4

Copyright EasyTier contributors. Distributed unmodified as separate executables under LGPL-3.0.

- Upstream: https://github.com/EasyTier/EasyTier/tree/v2.6.4
- Official binary archive SHA-256: `27af91e270e554709b048bd32327fefd2dfce5062ae1e8701af7550c6f525f84`
- License texts: `resources/licenses/EasyTier-LGPL-3.0.txt` and `resources/licenses/GPL-3.0.txt`.
- Matching upstream source, including build instructions and Cargo.lock: `resources/sources/EasyTier-v2.6.4-source.zip`.
- Localer launches `resources/easytier/easytier-core.exe` and `easytier-cli.exe` as separate processes. These can be replaced with interface-compatible modified builds in the unpacked distribution. No linking or modification of the engine is performed.
- For the portable self-extracting build, use the `win-unpacked` folder in the developer distribution, or extract the portable archive with 7-Zip, to replace these files. Localer source and build scripts are supplied in the project.

## Wintun 0.14.1

Copyright (c) 2018–2021 WireGuard LLC. The official signed amd64 DLL is distributed under the binary redistribution license supplied with Wintun. See `resources/licenses/Wintun-LICENSE.txt`.

Official distribution: https://www.wintun.net/builds/wintun-0.14.1.zip

The Npcap Packet.dll and WinDivert driver from the EasyTier release archive are **not** included in Localer's packaged resources. Physical-network broadcast capture is not enabled. Rebuilt legacy packages include the BSD WinPcap Packet API DLL solely to satisfy the engine's required loader dependency, with notices in resources/licenses/WinPcap-LICENSE.html. No WinPcap capture driver is installed. This product includes software developed by the University of California, Berkeley and its contributors; the Kungliga Tekniska Högskolan and its contributors; and Yen Yen Lim and North Dakota State University.

## Electron 44.9.0

Copyright Electron contributors and GitHub Inc. Electron's license and Chromium's third-party notices are included beside the application in `LICENSE.electron.txt` and `LICENSES.chromium.html` (inside the portable payload).

Development/build dependencies retain their respective upstream licenses in node_modules; these tools are not part of the runtime application.
