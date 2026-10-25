# Localer

Windows x64 için oda koduyla katılınan sanal özel ağ uygulaması. Türkçe masaüstü arayüzü, gerçek EasyTier 2.6.4 ağ motorunu yönetir.

## Kullanım

1. `dist/Localer-0.2.1-Windows-x64.exe` dosyasını sen ve arkadaşların kendi bilgisayarlarınızda açın. Windows'un yönetici iznini onaylayın. Node.js kurulumu gerekmez.
2. **Oda oluştur** düğmesine bas ve **Davet bağlantısını kopyala** ile arkadaşlarına ilet.
3. Arkadaşların **Odaya katıl** sekmesine bağlantıyı yapıştırsın. Varsayılan sunucuyu kullanıyorsanız yalnızca `LCL-…` kodunu da paylaşabilirsiniz.
4. Ekranda sanal IP göründükten sonra oyun veya servise bu IP ve portuyla bağlanın. Örnek: `10.x.y.z:25565` veya `http://10.x.y.z:5173`.
5. **Odadan ayrıl** bağlantıyı kapatır. Uygulamayı kapatmak da ağ motorunu durdurur.

Bu ilk sürüm kod imzalı değildir. Windows yayıncıyı doğrulayamaz. Kaynak kodu ve yeniden üretilebilir kurulum komutları bu projededir.

## Oyun ve localhost servisleri

- Oyunlarda **Doğrudan bağlan / Direct connect** alanına sunucuyu açan kişinin sanal IP ve portunu yazın.
- Yerel servis yalnızca `127.0.0.1` veya `localhost` üzerinde dinliyorsa uzaktan erişilemez. `0.0.0.0` veya sanal IP üzerinde dinletin. Örneğin Vite için `npm run dev -- --host 0.0.0.0`.
- Windows güvenlik duvarında ilgili uygulama veya port için gereken izni verin; mümkünse kuralı oda subnetiyle sınırlandırın. Güvenlik duvarını kapatmayın.
- Cihazın listede görünmesi, sunduğu servisin güvenlik duvarından geçebildiğini göstermez. Localer'ın yönetici izni diğer programların portlarını açmaz. Servisi çalıştıran program veya gereken TCP/UDP portu için gelen bağlantı izni gerekir.
- Bu, IP seviyesinde bir sanal ağdır. Fiziksel Ethernet ağıyla birebir aynı değildir; bazı oyunların otomatik LAN keşfi ve multicast tabanlı servis keşfi çalışmayabilir. Doğrudan IP bağlantısı kullanın.
- Cihazların internet çıkışı ortak olmaz; tüm internet trafiği oda sahibinden geçmez. Fiziksel ev ağının tamamı paylaşılmaz.

## Davet ve oda davranışı

Oda kodu işletim sisteminin kriptografik rastgele üretecinden 130 bit rastgelelikle üretilir. İstemciler koddan aynı ağ kimliği ve ayrı bir alanla türetilmiş SHA-256 ağ anahtarı hesaplar. Şifreli veri aktarımı ve NAT geçişi EasyTier'a aittir. Anahtar komut satırına yazılmaz; alt sürecin ortamında aktarılır. Oda kimlik bilgileri diske kaydedilmez; uygulama belleğinde ve açık davette bulunur.

Kodu bilenler odaya girebilir. Hesap, kişi çıkarma, süre dolumu ve davet iptali yoktur. Gerektiğinde yeni oda oluşturun. Odayı oluşturan kişinin diğerleri üzerinde yönetici rolü yoktur. Oluşturan kişi ayrıldığında diğer üyeler bağlı kalabilir; onun sunduğu servise erişim sona erer. Oda adı yalnızca yerel bir etikettir.

## Buluşma sunucusu

Varsayılan `tcp://38.76.179.190:11010`, resmi EasyTier/etcat projesinin [relays.toml](https://github.com/EasyTier/etcat/blob/main/relays.toml) kaydında yayımlanan, üçüncü tarafın işlettiği topluluk düğümüdür. Önceki `public.easytier.top` ve `public.easytier.cn` adresleri test sırasında DNS çözümlemediği için kaldırıldı. Yeni düğümle el sıkışma ve iki test istemcisi arasında zorunlu sunucu aktarımıyla gerçek TCP/UDP veri aktarımı 18 Ekim 2026 tarihinde doğrulandı. Aynı testte yanlış oda anahtarı reddedildi. Sunucunun sürekli açık kalması, kapasitesi ve gecikmesi garanti edilmez; Türkiye'den testte sunucu gecikmesi yaklaşık 227 ms idi. NAT geçişi başarılıysa istemciler doğrudan bağlanabilir.

Kendi internete açık EasyTier sunucun varsa **Bağlantı ayarları** bölümüne `tcp://SUNUCU:11010` yaz. Özel sunucu adresi kopyalanan davet bağlantısına gömülür; arkadaşların tam bağlantıyı yapıştırsın. Sunucu kurulumu bu projede otomatik yapılmaz. Türkiye'ye yakın bir sunucu gecikmeyi azaltabilir; sonuç rota ve NAT koşullarına bağlıdır.

Arayüzde **Arkadaşların bekleniyor**, yerel IP ve sunucu bağlantısının hazır olduğunu; **Bağlandı**, en az bir arkadaş cihazına bağlantı bulunduğunu gösterir. **Sunucuya ulaşılamıyor** durumunda motor yeniden denemeye devam eder; odadan ayrılıp sunucuyu değiştirebilirsin. Bağlantı kurulurken **Odadan ayrıl** işlemi bekleyen başlangıcı iptal eder. Her oturum ayrı bir `Localer-<port>` bağdaştırıcı adı kullanır. Yanıtsız motor sorguları veya bağdaştırıcı zaman aşımı, süresiz bekleme yerine açıklayıcı hata verir.

Yeni davet bağlantıları varsayılan dahil sunucu adresini taşır. Kısa kodu kullanacaksanız herkesin aynı sunucuyu seçmesi gerekir. Özel sunucuda arayüz yalnızca tam davet bağlantısının kopyalanmasını sunar. Eski, kaldırılmış resmi sunucu adreslerini taşıyan davetler güncel varsayılana geçirilir.

## Geliştirme

Windows x64 ve güncel Node.js gerekir. PowerShell:

```powershell
npm ci
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/setup-engine.ps1
npx install-electron --no
npm start
```

Uygulamadaki yönetici olarak yeniden aç düğmesi ağ bağdaştırıcısı için yükseltme ister. Geliştirme sırasında yönetici yetkisi sadece gerçek Windows sanal bağdaştırıcı testi için gerekir. Bu test için yönetici PowerShell penceresinde npm run test:tun komutunu çalıştırın.

İzin penceresi görünmüyorsa proje klasöründeki `Localer-Ag-Testi.cmd` dosyasına Dosya Gezgini'nden çift tıklayın. Bu başlatıcı görünür bir PowerShell penceresi açar ve standart Windows yönetici iznini ister. İzin verildikten sonra test ve sonuçlar aynı pencerede gösterilir. Pencereyi test bitene kadar açık bırakın; ardından Enter ile kapatabilirsiniz. UAC veya güvenlik duvarı ayarlarını değiştirmeniz gerekmez.

```powershell
npm test
npm run test:engine
npm run test:relay
npm run test:ui
npm run build
```

- `npm test`: davet üretme, ayrıştırma, anahtar türetme, güvenli süreç argümanları.
- `test:engine`: gerçek motorla loopback üzerinde aynı anahtarla katılma ve yanlış anahtarın reddedilmesi; TUN veya sistem yönlendirmesi oluşturmaz. Ek aktarım sonuçları `artifacts/engine-smoke.json` dosyasına yazılır.
- `test:relay`: internetteki varsayılan topluluk sunucusu üzerinden iki test istemcisiyle TCP/UDP aktarımı; doğrudan P2P kapalıdır. `artifacts/relay-transfer-smoke.json` üretir. Dış sunucuya geçici oda kimliği ve bağlantı bilgisi gönderir.
- `test:tun`: yönetici yetkisiyle gerçek Windows bağdaştırıcısını oluşturur; rootless arkadaş istemcisinden TCP/UDP isteğini sunucu üzerinden gerçek TUN'a ulaştırır, sonra kapatma ve IP kaldırılmasını kontrol eder. `artifacts/tun-smoke.json` üretir. Güvenlik duvarı kuralı eklemez.
- `test:tun:scoped`: önce kuralları değiştirmeden aynı testi yapar; gerekirse yalnızca test uygulaması, test arayüzü, test sanal IP adresleri ve geçici TCP/UDP portlarına izin ekleyip testi tekrarlar. İki geçici kural test sonunda kaldırılır ve yoklukları kontrol edilir. Yönetici PowerShell'de `npm run test:tun:scoped` veya görünür başlatıcıyla `Localer-Ag-Testi.cmd -AllowTestPorts` kullanılır. Uygulamanın normal kullanımı bu test kurallarını eklemez.
- `test:ui`: gerçek Electron penceresinde IPC ve sekme kontrolü; ekran görüntüsü `artifacts/ui.png`.
- `build`: bağımsız taşınabilir EXE ve `dist/win-unpacked` üretir.

18 Ekim 2026 tarihinde gerçek Windows bağdaştırıcısı oluşturma, DHCP ile IP alma, arkadaş istemcisini görme, sunucu üzerinden gerçek TUN ile TCP/UDP aktarımı ve ayrılınca IP kaldırılması doğrulandı. Bu bilgisayarda UDP doğrudan geçti; TCP, Windows güvenlik duvarında yalnızca test uygulaması, test arayüzü, test sanal IP çiftleri ve iki geçici servis portuna izin verildikten sonra geçti. Test kuralları kaldırıldı ve sanal IP temizliği doğrulandı. Mevcut kurallar ve UAC ayarları değiştirilmedi. Rapor: artifacts/tun-smoke.json. 25 Ekim 2026 tarihinde 15 mantık testi, arayüz kontrolleri ve internetteki aynı sunucu üzerinden TCP/UDP aktarımı yeniden geçti. Farklı internetlerde iki fiziksel bilgisayarla tam oyun testi henüz yapılmadı.

## Proje

- `src/main.js`: güvenli masaüstü pencere, sınırlı IPC, yetki kontrolü ve kapanış.
- `src/engine.js`: ağ motoru yaşam döngüsü, gerçek bağdaştırıcı kontrolü, katılımcılar ve tanılama.
- `src/room.js`: davetler, anahtarlar, sunucu doğrulama.
- `src/ui/`: çevrimdışı Türkçe arayüz; uzaktan içerik veya font indirmez.
- `scripts/setup-engine.ps1`: sabit sürümlü resmi arşivlerin SHA-256 doğrulaması ve lisanslar.

Localer arayüzü MIT; ağ motoru ve sürücü kendi lisanslarına tabidir. Ayrıntı: `THIRD-PARTY-NOTICES.md`.
