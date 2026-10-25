# Localer 0.3.0 Qt — Test sonuçları

Kontrol tarihi: 25 Ekim 2026.

| Kontrol | Sonuç | Kanıt |
| --- | --- | --- |
| C++ derleme | Geçti; Qt 6.10.3 / MinGW 13.1 / Release | CMake/Ninja |
| Electron davet protokolü uyumu | 32 vektörde ağ adı, anahtar, subnet ve davet birebir aynı | native/tests/protocol-vectors.json |
| Mantık ve hata durumları | 12 test durumu geçti | localer_tests |
| Gerçek Windows Qt arayüzü | Sekmeler, panoya kopyalama, düz metin cihaz adları, özel sunucu daveti ve ayrılma geçti | artifacts/qt/ui-smoke.json |
| Qt/C++ süreç yönetimiyle sunucudan TCP/UDP | Geçti; doğrudan P2P kapalı | artifacts/qt/network-smoke.json |
| Farklı oda yalıtımı | Geçti | artifacts/qt/network-smoke.json |
| Geliştirme araçları olmayan çalışma ortamı | Qt SDK, Python ve Node PATH'ten çıkarılınca arayüz ve ağ testleri geçti | artifacts/qt/portable/ |
| Gerçek Windows TUN | Bağdaştırıcı, sanal IP ve gerçek TCP/UDP aktarımı geçti | artifacts/qt/qt-tun-smoke.json |
| Kapanış temizliği | Test süreçleri kapandı; sanal IP ve iki geçici güvenlik duvarı kuralı kaldırıldı | artifacts/qt/qt-tun-smoke.json |
| Paket bağımlılıkları | Zorunlu Packet.dll eksikliği BSD uyumlu DLL ile giderildi; motor paket klasöründen çalıştı | dağıtım ve taşınabilir ortam testi |

Gerçek TUN testinde yalnızca test uygulaması, test arayüzü, iki test IP'si ve iki geçici servis portuna dar kapsamlı güvenlik duvarı izni verildi. İzinler sonunda kaldırıldı; mevcut kurallar ve UAC politikası değiştirilmedi. Localer normal kullanımda servis portlarını açmaz; paylaşılan oyun/servis için gereken Windows izni ayrıca verilmelidir.

Qt arayüzü ve süreç yönetimi C++'tır; Electron, Chromium, Node.js ve Python paket içinde çalışma zamanı olarak yoktur. Testler geliştirme hedefleridir, kullanıcı uygulaması bunları içermez. Dağıtımda LGPL/BSD lisansları ve eşleşen Qt, EasyTier, Localer kaynakları vardır.

Henüz doğrulanmayan: farklı internetlerde iki fiziksel bilgisayarla gerçek oyun oturumu. Topluluk sunucusu kapasitesi, sürekliliği ve gecikmesi garanti edilmez. Qt'ye geçiş ağ gecikmesini otomatik olarak düşürmez.
