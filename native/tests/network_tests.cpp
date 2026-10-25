#include "engine.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QScopeGuard>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QtTest>

class NetworkTests : public QObject {
    Q_OBJECT
private slots:
    void realQtEngineRelayTransfer() {
        const QString dir=qEnvironmentVariable("LOCALER_ENGINE_DIR",QCoreApplication::applicationDirPath()+"/easytier");
        QTcpServer http;
        QVERIFY(http.listen(QHostAddress::LocalHost,0));
        connect(&http,&QTcpServer::newConnection,this,[&http] {
            while (http.hasPendingConnections()) {
                auto *socket=http.nextPendingConnection();
                connect(socket,&QTcpSocket::readyRead,socket,[socket] {
                    socket->readAll(); socket->write("HTTP/1.1 200 OK\r\nContent-Length: 13\r\nConnection: close\r\n\r\nqt-overlay-ok"); socket->disconnectFromHost();
                });
                connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            }
        });
        QUdpSocket echo; QVERIFY(echo.bind(QHostAddress::LocalHost,0));
        connect(&echo,&QUdpSocket::readyRead,this,[&echo] {
            while (echo.hasPendingDatagrams()) { QByteArray buffer; buffer.resize(echo.pendingDatagramSize()); QHostAddress address; quint16 port; echo.readDatagram(buffer.data(),buffer.size(),&address,&port); echo.writeDatagram(buffer,address,port); }
        });
        QTcpServer tcpReservation; QVERIFY(tcpReservation.listen(QHostAddress::LocalHost,0)); const auto forwardTcp=tcpReservation.serverPort(); tcpReservation.close();
        QUdpSocket udpReservation; QVERIFY(udpReservation.bind(QHostAddress::LocalHost,0)); const auto forwardUdp=udpReservation.localPort(); udpReservation.close();
        const Rooms::Room room{Rooms::createCode(),Rooms::defaultRelay};
        Engine host(dir),friendEngine(dir),otherRoom(dir);
        const auto diagnostics=qEnvironmentVariable("LOCALER_ARTIFACT_DIR",QDir::currentPath()+"/artifacts/qt"); QDir().mkpath(diagnostics);
        const auto record=[&host,&friendEngine,diagnostics] {
            QJsonObject value;
            value["hostStatus"]=host.state().status; value["friendStatus"]=friendEngine.state().status;
            value["hostError"]=host.state().error; value["friendError"]=friendEngine.state().error;
            value["hostLogs"]=host.state().logs.join('\n'); value["friendLogs"]=friendEngine.state().logs.join('\n');
            QFile file(diagnostics+"/network-diagnostics.json"); if (file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(value).toJson());
        };
        connect(&host,&Engine::changed,this,record); connect(&friendEngine,&Engine::changed,this,record);
        auto disconnectDiagnostics=qScopeGuard([this,&host,&friendEngine] {
            disconnect(&host,nullptr,this,nullptr); disconnect(&friendEngine,nullptr,this,nullptr);
        });
        EngineOptions hostOptions; hostOptions.noTun=true; hostOptions.dhcp=false; hostOptions.ipv4Override="10.90.1.1/24"; hostOptions.disableP2p=true; hostOptions.useSmoltcp=true; hostOptions.pollMilliseconds=400;
        auto friendOptions=hostOptions; friendOptions.ipv4Override="10.90.1.2/24";
        friendOptions.forwards={QString("tcp://127.0.0.1:%1/10.90.1.1:%2").arg(forwardTcp).arg(http.serverPort()),QString("udp://127.0.0.1:%1/10.90.1.1:%2").arg(forwardUdp).arg(echo.localPort())};
        host.start(room,"qt-test-host",hostOptions); friendEngine.start(room,"qt-test-friend",friendOptions);
        QTRY_VERIFY_WITH_TIMEOUT(host.state().status=="connected" || host.state().status=="error",35000);
        QVERIFY2(host.state().status=="connected",qPrintable(host.state().error+"\n"+host.state().logs.join('\n')));
        QTRY_VERIFY_WITH_TIMEOUT(friendEngine.state().status=="connected" || friendEngine.state().status=="error",35000);
        QVERIFY2(friendEngine.state().status=="connected",qPrintable(friendEngine.state().error));
        QNetworkAccessManager network; auto *reply=network.get(QNetworkRequest(QUrl(QString("http://127.0.0.1:%1").arg(forwardTcp))));
        QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(),15000); QCOMPARE(reply->error(),QNetworkReply::NoError); QCOMPARE(reply->readAll(),QByteArray("qt-overlay-ok")); reply->deleteLater();
        QUdpSocket client; QVERIFY(client.bind(QHostAddress::LocalHost,0));
        QTimer send; send.setInterval(250); connect(&send,&QTimer::timeout,&client,[&client,forwardUdp]{client.writeDatagram("qt-udp-ok",9,QHostAddress::LocalHost,forwardUdp);}); send.start();
        QTRY_VERIFY_WITH_TIMEOUT(client.hasPendingDatagrams(),10000); send.stop(); QByteArray buffer(64,'\0'); const auto count=client.readDatagram(buffer.data(),buffer.size()); buffer.resize(count); QCOMPARE(buffer,QByteArray("qt-udp-ok"));
        auto isolatedOptions=hostOptions; isolatedOptions.ipv4Override="10.91.1.1/24"; otherRoom.start({Rooms::createCode(),Rooms::defaultRelay},"qt-other-room",isolatedOptions);
        QTRY_COMPARE_WITH_TIMEOUT(otherRoom.state().status,QString("waiting"),35000);
        for (const auto &peer : host.state().peers) QVERIFY(peer.name!="qt-other-room");
        auto *hostProcess=host.findChild<QProcess*>(); QVERIFY(hostProcess); QSignalSpy exitSpy(hostProcess,qOverload<int,QProcess::ExitStatus>(&QProcess::finished));
        host.stop(); friendEngine.stop(); otherRoom.stop();
        QCOMPARE(host.processId(),0); QCOMPARE(friendEngine.processId(),0); QCOMPARE(otherRoom.processId(),0); QVERIFY(!exitSpy.isEmpty());
        const auto artifacts=qEnvironmentVariable("LOCALER_ARTIFACT_DIR",QDir::currentPath()+"/artifacts/qt"); QDir().mkpath(artifacts);
        QFile report(artifacts+"/network-smoke.json"); QVERIFY(report.open(QIODevice::WriteOnly));
        report.write(QJsonDocument(QJsonObject{{"passed",true},{"framework","Qt QProcess"},{"relay",Rooms::defaultRelay},{"tcp",true},{"udp",true},{"roomIsolation",true},{"processCleanup",true},{"tunCreated",false},{"testedAt",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}}).toJson());
    }
};
QTEST_GUILESS_MAIN(NetworkTests)
#include "network_tests.moc"
