#include "engine.h"
#include "platform.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QScopeGuard>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QUuid>
#include <QtTest>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
QString quote(QString value) { return "'"+value.replace("'","''")+"'"; }
QString powershell(const QString &command,bool *success) {
    QProcess process;
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args){args->flags|=CREATE_NO_WINDOW;});
#endif
    process.start("powershell.exe",{"-NoProfile","-NonInteractive","-Command",command});
    *success=process.waitForFinished(20000) && process.exitCode()==0;
    if (process.state()!=QProcess::NotRunning) {process.kill();process.waitForFinished(1000);}
    const auto output=QString::fromUtf8(process.readAllStandardOutput());
    const auto error=QString::fromUtf8(process.readAllStandardError());
    return *success?output:output+"\n"+error;
}
bool containsAddress(const QString &address) {
    for (const auto &ip : QNetworkInterface::allAddresses()) if (ip.toString()==address) return true;
    return false;
}
}

class TunTests : public QObject {
    Q_OBJECT
private slots:
    void actualWindowsAdapter() {
        QVERIFY2(Platform::isAdministrator(),"Run this test from an administrator PowerShell window.");
        const QString directory=qEnvironmentVariable("LOCALER_ENGINE_DIR",QCoreApplication::applicationDirPath()+"/easytier");
        const auto artifacts=qEnvironmentVariable("LOCALER_ARTIFACT_DIR",QDir::currentPath()+"/artifacts/qt"); QDir().mkpath(artifacts);
        Engine host(directory),friendEngine(directory);
        const QString prefix="Localer-Qt-Test-"+QUuid::createUuid().toString(QUuid::WithoutBraces);
        QString hostIp;
        QJsonObject report{{"passed",false},{"framework","Qt / C++ production engine"},{"startedAt",QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},{"relay",Rooms::defaultRelay}};
        bool cleanupSucceeded=false;
        auto cleanup=qScopeGuard([&] {
            friendEngine.stop(); host.stop();
            const QString names=quote(prefix+"-TCP")+","+quote(prefix+"-UDP");
            bool removed=false;
            const auto result=powershell("Get-NetFirewallRule -Name "+names+" -ErrorAction SilentlyContinue | Remove-NetFirewallRule -ErrorAction Stop; @(Get-NetFirewallRule -Name "+names+" -ErrorAction SilentlyContinue).Count",&removed);
            const bool rulesRemoved=removed && result.trimmed()=="0";
            report["firewallRulesRemoved"]=rulesRemoved; report["virtualIpRemoved"]=!hostIp.isEmpty() && !containsAddress(hostIp);
            report["finishedAt"]=QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
            report["passed"]=cleanupSucceeded && rulesRemoved && !containsAddress(hostIp);
            QFile file(artifacts+"/qt-tun-smoke.json"); if (file.open(QIODevice::WriteOnly)) file.write(QJsonDocument(report).toJson());
        });
        const Rooms::Room room{Rooms::createCode(),Rooms::defaultRelay};
        EngineOptions hostOptions; hostOptions.pollMilliseconds=500;
        host.start(room,"qt-real-tun-host",hostOptions);
        QTRY_VERIFY_WITH_TIMEOUT(!host.state().ip.isEmpty() || host.state().status=="error",35000);
        QVERIFY2(!host.state().ip.isEmpty(),qPrintable(host.state().error));
        hostIp=host.state().ip; QVERIFY(containsAddress(hostIp)); report["realAdapter"]=true;
        QTcpServer service; QVERIFY(service.listen(QHostAddress(hostIp),0));
        connect(&service,&QTcpServer::newConnection,this,[&service]{
            while (service.hasPendingConnections()) {
                auto *socket=service.nextPendingConnection();
                connect(socket,&QTcpSocket::readyRead,socket,[socket]{socket->readAll();socket->write("HTTP/1.1 200 OK\r\nContent-Length: 9\r\nConnection: close\r\n\r\nqt-tun-ok");socket->disconnectFromHost();});
                connect(socket,&QTcpSocket::disconnected,socket,&QObject::deleteLater);
            }
        });
        QUdpSocket echo; QVERIFY(echo.bind(QHostAddress(hostIp),0));
        connect(&echo,&QUdpSocket::readyRead,this,[&echo]{
            while (echo.hasPendingDatagrams()) {QByteArray bytes;bytes.resize(echo.pendingDatagramSize());QHostAddress source;quint16 port;echo.readDatagram(bytes.data(),bytes.size(),&source,&port);echo.writeDatagram(bytes,source,port);}
        });
        const auto friendIp=hostIp.section('.',0,2)+".250";
        const auto common=" -Direction Inbound -Action Allow -Profile Any -Program "+quote(QDir::toNativeSeparators(QCoreApplication::applicationFilePath()))+" -InterfaceAlias "+quote(QString("Localer-%1").arg(host.rpcPort()))+" -LocalAddress "+quote(hostIp)+" -RemoteAddress "+quote(friendIp);
        bool allowed=false;
        const auto firewallResult=powershell("$ErrorActionPreference='Stop'; New-NetFirewallRule -Name "+quote(prefix+"-TCP")+" -DisplayName 'Localer temporary Qt TCP test'"+common+" -Protocol TCP -LocalPort "+QString::number(service.serverPort())+" | Out-Null; New-NetFirewallRule -Name "+quote(prefix+"-UDP")+" -DisplayName 'Localer temporary Qt UDP test'"+common+" -Protocol UDP -LocalPort "+QString::number(echo.localPort())+" | Out-Null",&allowed);
        if (!allowed) report["firewallError"]=firewallResult;
        QVERIFY2(allowed,qPrintable(firewallResult)); report["usedScopedTestRules"]=true;
        QTcpServer tcpReservation; QVERIFY(tcpReservation.listen(QHostAddress::LocalHost,0));const auto tcpPort=tcpReservation.serverPort();tcpReservation.close();
        QUdpSocket udpReservation; QVERIFY(udpReservation.bind(QHostAddress::LocalHost,0));const auto udpPort=udpReservation.localPort();udpReservation.close();
        EngineOptions friendOptions;friendOptions.noTun=true;friendOptions.dhcp=false;friendOptions.ipv4Override=friendIp+"/24";friendOptions.disableP2p=true;friendOptions.pollMilliseconds=500;
        friendOptions.forwards={QString("tcp://127.0.0.1:%1/%2:%3").arg(tcpPort).arg(hostIp).arg(service.serverPort()),QString("udp://127.0.0.1:%1/%2:%3").arg(udpPort).arg(hostIp).arg(echo.localPort())};
        friendEngine.start(room,"qt-real-tun-friend",friendOptions);
        QTRY_COMPARE_WITH_TIMEOUT(host.state().status,QString("connected"),35000);
        QNetworkAccessManager network;auto *reply=network.get(QNetworkRequest(QUrl(QString("http://127.0.0.1:%1").arg(tcpPort))));
        QTRY_VERIFY_WITH_TIMEOUT(reply->isFinished(),15000); QCOMPARE(reply->error(),QNetworkReply::NoError);QCOMPARE(reply->readAll(),QByteArray("qt-tun-ok"));reply->deleteLater();report["tcp"]=true;
        QUdpSocket client;QVERIFY(client.bind(QHostAddress::LocalHost,0));QTimer retry;retry.setInterval(250);
        connect(&retry,&QTimer::timeout,&client,[&client,udpPort]{client.writeDatagram("qt-tun-udp",10,QHostAddress::LocalHost,udpPort);});retry.start();
        QTRY_VERIFY_WITH_TIMEOUT(client.hasPendingDatagrams(),10000);retry.stop();QByteArray bytes(64,'\0');bytes.resize(client.readDatagram(bytes.data(),bytes.size()));QCOMPARE(bytes,QByteArray("qt-tun-udp"));report["udp"]=true;
        service.close();echo.close();friendEngine.stop();host.stop();
        QTRY_VERIFY_WITH_TIMEOUT(!containsAddress(hostIp),5000);report["processCleanup"]=host.processId()==0 && friendEngine.processId()==0;
        cleanupSucceeded=true;
    }
};
QTEST_GUILESS_MAIN(TunTests)
#include "tun_tests.moc"
