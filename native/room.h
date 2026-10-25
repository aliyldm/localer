#pragma once
#include <QString>

namespace Rooms {
inline const QString defaultRelay = QStringLiteral("tcp://38.76.179.190:11010");
struct Room { QString code; QString relay = defaultRelay; };
struct Credentials { QString name; QString secret; QString subnet; };
QString createCode();
QString normalizeCode(const QString &input);
QString validateRelay(const QString &input);
Room parseInvite(const QString &input, const QString &fallbackRelay = defaultRelay);
Credentials credentials(const QString &code);
QString inviteLink(const Room &room);
}
