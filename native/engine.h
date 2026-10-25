#pragma once
#include "room.h"
#include <QElapsedTimer>
#include <QJsonArray>
#include <QObject>
#include <QProcess>
#include <QTimer>

struct Peer {
    QString name, ip, connection;
    bool local=false;
    double latency=-1;
};
struct NetworkState {
    QString status="idle", ip, error, message;
    Rooms::Room room;
    bool hasRoom=false, relayConnected=false;
    QList<Peer> peers;
    QStringList logs;
};
struct EngineOptions {
    bool noTun=false, dhcp=true, disableP2p=false, useSmoltcp=false;
    QString ipv4Override;
    QStringList forwards;
    int pollMilliseconds=2500;
};

class Engine : public QObject {
    Q_OBJECT
public:
    explicit Engine(QString directory, QObject *parent=nullptr);
    ~Engine() override;
    const NetworkState &state() const { return m_state; }
    qint64 processId() const { return m_core ? m_core->processId() : 0; }
    quint16 rpcPort() const { return m_port; }
    void start(const Rooms::Room &room, const QString &nickname, const EngineOptions &options={});
    void stop();
    void poll();
    static QStringList arguments(const Rooms::Room &room, quint16 port, const QString &nickname, const EngineOptions &options={});
    static QList<Peer> normalizePeers(const QJsonArray &rows);
    static NetworkState evaluate(const NetworkState &previous, const QJsonArray &rows, const QStringList &localAddresses, qint64 elapsed, bool noTun=false);
signals:
    void changed();
private:
    void appendLog(const QString &text);
    void fail(const QString &error);
    void queryFinished(QProcess *query, quint64 generation, bool success);
    void stopProcess(QProcess *&process);
    QString m_directory, m_secret;
    NetworkState m_state;
    QProcess *m_core=nullptr, *m_query=nullptr;
    QTimer m_pollTimer, m_queryTimer;
    QElapsedTimer m_elapsed;
    EngineOptions m_options;
    quint64 m_generation=0;
    quint16 m_port=0;
    int m_failures=0;
    void *m_job=nullptr;
};
