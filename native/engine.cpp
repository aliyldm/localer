#include "engine.h"
#include "platform.h"
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTimeZone>
#include <stdexcept>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {
QString clean(QString text) { return text.remove(QRegularExpression(QStringLiteral("\\x1b\\[[0-9;]*m"))).trimmed(); }
bool ipv4(const QString &text) { return QHostAddress(text.section('/',0,0)).protocol()==QAbstractSocket::IPv4Protocol; }
QProcessEnvironment environment(const QString &secret={}) {
    auto env=QProcessEnvironment::systemEnvironment();
    for (const auto &key : env.keys()) if (key.startsWith("ET_",Qt::CaseInsensitive)) env.remove(key);
    env.insert("RUST_LOG","warn");
    if (!secret.isEmpty()) env.insert("ET_NETWORK_SECRET",secret);
    return env;
}
void hideChild(QProcess *process) {
#ifdef Q_OS_WIN
    process->setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags|=CREATE_NO_WINDOW;
        args->startupInfo->dwFlags|=STARTF_USESHOWWINDOW;
        args->startupInfo->wShowWindow=SW_HIDE;
    });
#else
    Q_UNUSED(process);
#endif
}
QStringList adapterAddresses() {
    QStringList addresses;
    for (const auto &adapter : QNetworkInterface::allInterfaces()) {
        if (!adapter.flags().testFlag(QNetworkInterface::IsUp)) continue;
        for (const auto &entry : adapter.addressEntries()) addresses.append(entry.ip().toString());
    }
    return addresses;
}
}

Engine::Engine(QString directory,QObject *parent):QObject(parent),m_directory(std::move(directory)) {
    m_queryTimer.setSingleShot(true); m_queryTimer.setInterval(4000);
    connect(&m_pollTimer,&QTimer::timeout,this,&Engine::poll);
    connect(&m_queryTimer,&QTimer::timeout,this,[this] {
        if (m_query) { auto *query=m_query; query->kill(); queryFinished(query,m_generation,false); }
    });
    m_job=Platform::createProcessJob();
}
Engine::~Engine() { stop(); Platform::closeProcessJob(m_job); }

QStringList Engine::arguments(const Rooms::Room &room,quint16 port,const QString &nickname,const EngineOptions &options) {
    const auto keys=Rooms::credentials(room.code);
    QStringList result={"--network-name",keys.name,"--ipv4",options.ipv4Override.isEmpty()?keys.subnet:options.ipv4Override,
        "--hostname",nickname,"--dev-name",QString("Localer-%1").arg(port),"--peers",Rooms::validateRelay(room.relay),
        "--listeners","tcp://0.0.0.0:0","udp://0.0.0.0:0","--rpc-portal",QString("127.0.0.1:%1").arg(port),
        "--rpc-portal-whitelist","127.0.0.1/32","--private-mode","true","--disable-upnp","--bind-device","false",
        "--accept-dns","false","--console-log-level","warn","--file-log-level","off"};
    if (options.dhcp) result.append("--dhcp");
    if (options.noTun) result.append("--no-tun");
    if (options.disableP2p) result.append("--disable-p2p");
    if (options.useSmoltcp) result.append("--use-smoltcp");
    for (const auto &forward : options.forwards) result.append({"--port-forward",forward});
    return result;
}

void Engine::start(const Rooms::Room &input,const QString &nickname,const EngineOptions &options) {
    if (m_core) throw std::runtime_error("Önce mevcut odadan ayrıl.");
    if (nickname.trimmed().isEmpty() || nickname.size()>32 || nickname.contains(QRegularExpression("[\\x00-\\x1f\\x7f]")))
        throw std::runtime_error("Cihaz adı 1–32 karakter olmalı.");
    const Rooms::Room room={Rooms::normalizeCode(input.code),Rooms::validateRelay(input.relay)};
    for (const auto &file : {"easytier-core.exe","easytier-cli.exe","wintun.dll","Packet.dll"})
        if (!QFileInfo::exists(QDir(m_directory).filePath(file))) throw std::runtime_error("Ağ motoru eksik. Uygulamayı ZIP dosyasından tamamen çıkart.");
    QTcpServer reservation;
    if (!reservation.listen(QHostAddress::LocalHost,0)) throw std::runtime_error("Yerel yönetim portu ayrılamadı.");
    m_port=reservation.serverPort(); reservation.close();
    const auto generation=++m_generation;
    m_options=options; m_secret=Rooms::credentials(room.code).secret; m_failures=0;
    m_state=NetworkState{}; m_state.room=room; m_state.hasRoom=true;
    m_state.status="connecting"; m_state.message="Sanal ağ hazırlanıyor…";
    appendLog("Ağ motoru başlatılıyor."); emit changed();
    m_elapsed.start();
    m_core=new QProcess(this);
    auto *core=m_core;
    core->setWorkingDirectory(m_directory); core->setProcessEnvironment(environment(m_secret)); hideChild(core);
    connect(core,&QProcess::readyReadStandardOutput,this,[this,core,generation] {
        if (generation==m_generation) appendLog(QString::fromUtf8(core->readAllStandardOutput()));
    });
    connect(core,&QProcess::readyReadStandardError,this,[this,core,generation] {
        if (generation==m_generation) appendLog(QString::fromUtf8(core->readAllStandardError()));
    });
    connect(core,&QProcess::started,this,[this,core,generation] {
        if (generation!=m_generation) return;
        Platform::addProcessToJob(m_job,core->processId());
        m_pollTimer.start(m_options.pollMilliseconds);
        QTimer::singleShot(100,this,[this,generation] { if (generation==m_generation) poll(); });
    });
    connect(core,&QProcess::errorOccurred,this,[this,core,generation](QProcess::ProcessError error) {
        if (generation==m_generation && error==QProcess::FailedToStart) { appendLog(core->errorString()); fail("Ağ motoru başlatılamadı. Dosyaları ve yönetici iznini kontrol et."); }
    });
    connect(core,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this,generation](int,QProcess::ExitStatus) {
        if (generation==m_generation) fail("Ağ motoru kapandı. Bağlantı günlüğünü kontrol edip tekrar dene.");
    });
    core->start(QDir(m_directory).filePath("easytier-core.exe"),arguments(room,m_port,nickname.trimmed(),options));
}

void Engine::appendLog(const QString &text) {
    QString line=clean(text);
    if (line.isEmpty()) return;
    if (!m_secret.isEmpty()) line.replace(m_secret,"[gizli]");
    if (!m_state.room.code.isEmpty()) line.replace(m_state.room.code,"[gizli]");
    m_state.logs.append("["+QDateTime::currentDateTimeUtc().toTimeZone(QTimeZone("Europe/Istanbul")).toString("HH:mm:ss")+"] "+line.left(1500));
    while (m_state.logs.size()>100) m_state.logs.removeFirst();
}

QList<Peer> Engine::normalizePeers(const QJsonArray &rows) {
    QList<Peer> peers;
    for (const auto &value : rows) {
        const auto row=value.toObject();
        const QString ip=row.value("ipv4").toString().section('/',0,0);
        if (!ipv4(ip)) continue;
        const auto cost=row.value("cost").toVariant().toString().toLower();
        bool number=false;
        const auto latency=row.value("lat_ms").toVariant().toString().toDouble(&number);
        peers.append({row.value("hostname").toString("İsimsiz cihaz").left(64),ip,
            cost=="local"?"Bu cihaz":cost=="p2p"?"Doğrudan":"Aktarmalı",cost=="local",number?latency:-1});
    }
    return peers;
}
NetworkState Engine::evaluate(const NetworkState &previous,const QJsonArray &rows,const QStringList &localAddresses,qint64 elapsed,bool noTun) {
    auto result=previous;
    const auto peers=normalizePeers(rows);
    result.ip.clear(); result.peers.clear(); result.relayConnected=false; result.error.clear();
    int remoteCount=0;
    for (const auto &peer : peers) {
        if (peer.local && (noTun || localAddresses.contains(peer.ip))) result.ip=peer.ip;
        if (!peer.local) remoteCount++;
    }
    for (const auto &value : rows) {
        const auto row=value.toObject();
        if (!ipv4(row.value("ipv4").toString()) && row.value("cost").toVariant().toString().compare("p2p",Qt::CaseInsensitive)==0)
            result.relayConnected=true;
    }
    result.status="connecting";
    result.message=result.ip.isEmpty()?"Sanal ağ bağdaştırıcısı hazırlanıyor…":"Sanal IP hazır; sunucuya bağlanılıyor…";
    if (!result.ip.isEmpty()) {
        result.peers=peers;
        if (remoteCount) { result.status="connected"; result.message=QString("%1 arkadaş cihazına bağlantı var.").arg(remoteCount); }
        else if (result.relayConnected) { result.status="waiting"; result.message="Oda hazır. Davet kodunu arkadaşlarınla paylaş."; }
        else if (elapsed>15000) { result.status="degraded"; result.message="Sunucuya ulaşılamıyor. Bağlantı yeniden deneniyor; gerekirse ayrılıp sunucuyu değiştir."; }
    }
    return result;
}

void Engine::poll() {
    if (!m_core || m_core->state()!=QProcess::Running) return;
    if (m_state.ip.isEmpty() && m_elapsed.elapsed()>35000) { fail("Sanal ağ hazırlanamadı. Yönetici iznini ve bağlantı günlüğünü kontrol et."); return; }
    if (m_query) return;
    const auto generation=m_generation;
    m_query=new QProcess(this);
    auto *query=m_query;
    hideChild(query); query->setWorkingDirectory(m_directory); query->setProcessEnvironment(environment());
    connect(query,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this,query,generation](int code,QProcess::ExitStatus status) {
        queryFinished(query,generation,code==0 && status==QProcess::NormalExit);
    });
    connect(query,&QProcess::errorOccurred,this,[this,query,generation](QProcess::ProcessError error) {
        if (error==QProcess::FailedToStart) queryFinished(query,generation,false);
    });
    query->start(QDir(m_directory).filePath("easytier-cli.exe"),{"-p",QString("127.0.0.1:%1").arg(m_port),"-o","json","peer"});
    m_queryTimer.start();
}
void Engine::queryFinished(QProcess *query,quint64 generation,bool success) {
    if (generation!=m_generation || query!=m_query) return;
    m_queryTimer.stop(); m_query=nullptr;
    const auto output=clean(QString::fromUtf8(query->readAllStandardOutput()));
    const auto queryError=QString::fromUtf8(query->readAllStandardError());
    query->disconnect(this);
    if (query->state()!=QProcess::NotRunning) query->kill();
    query->deleteLater();
    QJsonParseError parseError;
    const auto json=QJsonDocument::fromJson(output.toUtf8(),&parseError);
    if (!success || parseError.error!=QJsonParseError::NoError) {
        appendLog("Durum sorgusu yanıtı okunamadı: "+queryError.left(300)+(parseError.error!=QJsonParseError::NoError?parseError.errorString():QString()));
        m_failures++;
        if (m_failures>=3 && m_elapsed.elapsed()>10000) { fail("Ağ motoru yanıt vermiyor. Yeniden bağlanmayı dene."); return; }
        m_state.status="connecting"; m_state.message="Ağ motorundan yanıt bekleniyor…";
        m_state.ip.clear(); m_state.peers.clear(); m_state.relayConnected=false;
    } else {
        m_failures=0;
        auto rows=json.isArray()?json.array():json.object().value("peers").toArray();
        if (rows.isEmpty() && json.isObject()) rows=json.object().value("peer_infos").toArray();
        m_state=evaluate(m_state,rows,adapterAddresses(),m_elapsed.elapsed(),m_options.noTun);
    }
    emit changed();
}
void Engine::stopProcess(QProcess *&process) {
    if (!process) return;
    auto *stopping=process; process=nullptr;
    stopping->disconnect(this);
    if (stopping->state()!=QProcess::NotRunning) { stopping->kill(); stopping->waitForFinished(2000); }
    stopping->deleteLater();
}
void Engine::stop() {
    ++m_generation; m_pollTimer.stop(); m_queryTimer.stop();
    stopProcess(m_query); stopProcess(m_core); m_secret.clear(); m_failures=0;
    const auto logs=m_state.logs;
    m_state=NetworkState{}; m_state.logs=logs;
    emit changed();
}
void Engine::fail(const QString &error) {
    appendLog(error); stop();
    m_state.status="error"; m_state.error=error; emit changed();
}
