#include "room.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <stdexcept>

namespace {
const QString alphabet = QStringLiteral("ABCDEFGHJKLMNPQRSTUVWXYZ23456789");
[[noreturn]] void fail(const char *message) { throw std::runtime_error(message); }
QString format(const QString &raw) {
    return QStringLiteral("LCL-%1-%2-%3-%4-%5").arg(raw.mid(0,5),raw.mid(5,5),raw.mid(10,5),raw.mid(15,5),raw.mid(20));
}
bool isLink(const QString &text) { return text.startsWith(QStringLiteral("localer://"), Qt::CaseInsensitive); }
}

QString Rooms::createCode() {
    QString raw;
    for (int i=0; i<26; ++i) raw += alphabet.at(QRandomGenerator::system()->generate() & 31);
    return format(raw);
}
QString Rooms::normalizeCode(const QString &input) {
    if (input.size()>1024) fail("Geçerli bir davet kodu gir.");
    QString raw = input.trimmed();
    if (isLink(raw)) {
        const QUrl url(raw, QUrl::StrictMode);
        if (!url.isValid()) fail("Davet bağlantısı geçersiz.");
        raw = url.path();
        if (raw.startsWith('/')) raw.remove(0,1);
    }
    raw = raw.toUpper();
    raw.remove(QRegularExpression(QStringLiteral("^LCL-?")));
    raw.remove(QRegularExpression(QStringLiteral("[\\s-]")));
    if (raw.size()!=26) fail("Davet kodu geçersiz. LCL ile başlayan kodun tamamını yapıştır.");
    for (const QChar character : raw) if (!alphabet.contains(character)) fail("Davet kodu geçersiz.");
    return format(raw);
}
QString Rooms::validateRelay(const QString &input) {
    if (input.isEmpty() || input.size()>256) fail("Buluşma sunucusu adresi geçersiz.");
    QUrl url(input.trimmed(), QUrl::StrictMode);
    const QString protocol = url.scheme().toLower();
    if (!url.isValid() || !QStringList{"tcp","udp","ws","wss"}.contains(protocol) || url.host().isEmpty()
        || !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment() || (!url.path().isEmpty() && url.path()!="/"))
        fail("Sunucu adresi tcp://adres:11010 biçiminde olmalı.");
    if ((protocol=="tcp" || protocol=="udp") && url.port(-1)<1) fail("Sunucu portunu belirt. Örnek: tcp://adres:11010");
    url.setScheme(protocol); url.setPath(QString());
    QString result = url.toString(QUrl::FullyEncoded);
    if (result.endsWith('/')) result.chop(1);
    return result;
}
Rooms::Room Rooms::parseInvite(const QString &input, const QString &fallbackRelay) {
    if (input.size()>1024) fail("Davet bağlantısı çok uzun.");
    QString relay = fallbackRelay.isEmpty() ? defaultRelay : fallbackRelay;
    if (isLink(input.trimmed())) {
        const QUrl url(input.trimmed(), QUrl::StrictMode);
        if (!url.isValid() || url.host().compare("join",Qt::CaseInsensitive)!=0 || !url.userInfo().isEmpty()
            || url.port(-1)!=-1 || url.hasFragment()) fail("Davet bağlantısı geçersiz.");
        relay = QUrlQuery(url).queryItemValue("relay",QUrl::FullyDecoded);
        if (relay.isEmpty()) relay = defaultRelay;
    }
    relay = validateRelay(relay);
    if (relay=="tcp://public.easytier.top:11010" || relay=="tcp://public.easytier.cn:11010") relay=defaultRelay;
    return {normalizeCode(input), relay};
}
Rooms::Credentials Rooms::credentials(const QString &code) {
    const auto canonical = normalizeCode(code);
    const auto hash = [&canonical](const char *domain) {
        return QString::fromLatin1(QCryptographicHash::hash((QString::fromLatin1(domain)+":"+canonical).toUtf8(),QCryptographicHash::Sha256).toHex());
    };
    const auto network = hash("localer-network-v1");
    return {"localer-"+network.left(24),hash("localer-secret-v1"),
        QStringLiteral("10.%1.%2.1/24").arg(64+network.mid(0,2).toInt(nullptr,16)%64).arg(network.mid(2,2).toInt(nullptr,16))};
}
QString Rooms::inviteLink(const Room &room) {
    // Match URLSearchParams encoding so old and new clients exchange identical invites.
    return "localer://join/"+normalizeCode(room.code)+"?relay="+QString::fromLatin1(QUrl::toPercentEncoding(validateRelay(room.relay)));
}
