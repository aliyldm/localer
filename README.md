# Localer 0.3.0 — Qt / C++

Windows x64 için sade açık temalı sanal ağ uygulaması. Qt 6 Widgets ve C++ kullanır. Node.js, Electron, Chromium ve Python çalışma zamanında gerekmez.

Bu depo uygulama kaynaklarını ve yeniden derleme betiklerini içerir. Qt SDK, indirilen üçüncü taraf motorlar ve üretilen Windows paketleri Git'e dahil değildir. Taşınabilir ZIP, aşağıdaki derleme adımlarıyla `dist` klasöründe oluşturulur. Projenin mevcut Apache 2.0 lisansı korunmuştur; üçüncü taraf bileşenlerin lisansları ayrıdır.

## Kullanım

1. ZIP dosyasını tamamen bir klasöre çıkart. `Localer.exe`, Qt DLL'leri ve `easytier` klasörü birlikte kalmalı.
2. `Localer.exe` dosyasını açıp Windows yönetici iznini ver.
3. **Oda oluştur** ile davet al ve arkadaşlarına gönder. Arkadaşların aynı pakette **Odaya katıl** ekranına daveti yapıştırsın.
4. Arkadaşın cihaz listesinde görününce oyun veya servise sanal IP ve portuyla bağlanın.
5. **Odadan ayrıl** veya pencereyi kapatmak ağ motorunu durdurur.

Electron 0.2.x ile oda kodları ve anahtar türetme biçimi aynıdır. Tam davet bağlantısı sunucu adresini de taşır. Kısa kod kullanıldığında iki tarafta aynı buluşma sunucusu seçili olmalıdır. İlk sürüm kod imzalı değildir.

## Oyun ve servis bağlantısı

Oyunlarda doğrudan IP bağlantısı kullan. Yerel web servisini `0.0.0.0` veya sanal IP üzerinde dinlet; arkadaşın `http://sanal-IP:port` adresini açsın. Yalnızca `localhost` dinleyen servis uzaktan açılamaz.

Windows güvenlik duvarında servis programına veya gerekli TCP/UDP portuna gelen bağlantı izni gerekebilir. Cihaz listesi, servis portunun açık olduğunu göstermez. İzni mümkünse odanın sanal ağıyla sınırla; güvenlik duvarını kapatma. Uygulama normal kullanımda güvenlik duvarı kuralları eklemez.

Otomatik LAN/multicast keşfi her oyunda çalışmayabilir; fiziksel Ethernet ağı birebir taklit edilmez. İnternet çıkışınız ortak olmaz. Odayı oluşturan kişi diğer üyelerin yöneticisi değildir. Kişi çıkarma veya davet iptali yoktur; gerekirse yeni oda oluştur. Kodu yalnızca güvendiğin kişilerle paylaş.

## Bağlantı ve dağıtım

Ağ motoru EasyTier 2.6.4'tür. Varsayılan üçüncü taraf topluluk düğümü `tcp://38.76.179.190:11010`, resmi EasyTier/etcat projesinin relays.toml kaydından alınmıştır. Ulaşılabilirliği ve gecikmesi garanti edilmez; kendi EasyTier sunucunu ayarlardan seçebilirsin. P2P kurulamazsa sunucudan aktarım kullanılabilir.

Qt arayüzü ile motor ayrı süreçlerdir. Yönetim portu sadece `127.0.0.1` üzerinde açılır. Anahtar komut satırına veya oda kaydı olarak diske yazılmaz; alt süreç ortamında taşınır. Pencere kapanınca süreçler durdurulur; Windows job nesnesi beklenmedik kapanışta motoru da kapatır.

EasyTier'ın zorunlu `Packet.dll` yükleme bağımlılığı, BSD lisanslı WinPcap Packet API DLL'siyle karşılanır. Yalnızca bu kullanıcı alanı DLL'si bulunur; eski WinPcap kurucusu, npf sürücüsü veya Npcap dağıtılmaz ve fiziksel ağ yakalama etkinleştirilmez. Asıl sanal bağdaştırıcı resmi imzalı Wintun 0.14.1'i kullanır.

## Kaynak ve derleme

`sources/Localer-0.3.0-source.zip` uygulama kaynakları ve derleme betiklerini içerir. Geliştirme için Python 3.10+, 7-Zip ve internet erişimi gerekir; son kullanıcıya bunlar gerekmez.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup-qt.ps1 -PythonExe python
powershell -ExecutionPolicy Bypass -File scripts/setup-engine.ps1
powershell -ExecutionPolicy Bypass -File scripts/setup-packet-api.ps1
powershell -ExecutionPolicy Bypass -File scripts/setup-qt-source.ps1
powershell -ExecutionPolicy Bypass -File scripts/build-qt.ps1
```

Qt ve derleyici `vendor` içinde kurulur. CMake, Qt 6.10.3 ve MinGW 13.1 sürümleri sabitlenmiştir. C++ ve QtTest kaynakları `native` altında; `localer_tests`, `localer_ui_tests`, `localer_network_tests`, `localer_tun_tests` test hedefleridir. Normal derleme mantık ve arayüz testlerini çalıştırır. Ağ testi dış sunucuya geçici oda bilgisi gönderir ve ayrıca çalıştırılır. Gerçek Windows bağdaştırıcı testi için `scripts/start-qt-tun-test.ps1` görünür bir yönetici izin penceresi açar; sadece test uygulaması, test arayüzü, iki test IP'si ve iki test portuna verilen geçici izinler sonunda kaldırılır. Mevcut kurallar ve UAC politikası değiştirilmez.

25 Ekim 2026 kontrollerinde oda kodlarının Electron ile uyumu, gerçek sunucudan TCP/UDP aktarımı, temiz çalışma ortamı, gerçek Windows bağdaştırıcısı ve kapanış temizliği geçti. Ayrıntılar: `TEST-SONUCLARI.md`. İki farklı internet üzerindeki fiziksel bilgisayarlarla oyun testi henüz yapılmadı.

Qt dinamik DLL'lerle bağlanır. Uyumlu değiştirilmiş Qt DLL'lerini aynı klasörde değiştirerek uygulamayı çalıştırabilirsiniz. Kütüphane değişiklikleri için tersine mühendislik veya yeniden bağlama kısıtlanmaz. Tam Qt ve EasyTier kaynak arşivleri `sources` altında, lisans metinleri `licenses` altında bulunur. Ayrıntılar: `THIRD-PARTY-NOTICES.md`.


## Korunan Electron sürümü

Önceki JavaScript uygulaması src altında, eski açıklamalar docs/ELECTRON-README.md dosyasında korunur. npm start, npm build ve npm test artık Qt sürümünü kullanır; Electron için start:electron, build:electron ve test:electron komutları vardır. Eski 0.2.x paketleri zorunlu Packet.dll dosyasını içermiyordu; son kullanıcı için yeni Qt 0.3.0 paketini kullanın.
