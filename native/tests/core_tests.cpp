#include "engine.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QTemporaryDir>
#include <QtTest>
#include <stdexcept>

class CoreTests : public QObject {
    Q_OBJECT
    QJsonArray rows(bool relay=true,bool friendPresent=false) {
        QJsonArray result{QJsonObject{{"hostname","Host"},{"ipv4","10.80.1.1"},{"cost","Local"},{"lat_ms","-"}}};
        if (relay) result.append(QJsonObject{{"hostname","Relay"},{"ipv4",""},{"cost","p2p"}});
        if (friendPresent) result.append(QJsonObject{{"hostname","Friend"},{"ipv4","10.80.1.2"},{"cost","relay(2)"},{"lat_ms","40.5"}});
        return result;
    }
private slots:
    void electronProtocolCompatibility() {
        QFile file(LOCALER_VECTOR_PATH); QVERIFY(file.open(QIODevice::ReadOnly));
        const auto vectors=QJsonDocument::fromJson(file.readAll()).array(); QCOMPARE(vectors.size(),32);
        for (const auto &value : vectors) {
            const auto vector=value.toObject(); const auto code=vector["code"].toString(); const auto keys=Rooms::credentials(code);
            QCOMPARE(keys.name,vector["name"].toString()); QCOMPARE(keys.secret,vector["secret"].toString()); QCOMPARE(keys.subnet,vector["subnet"].toString());
            QCOMPARE(Rooms::normalizeCode(code.toLower()),code);
            QCOMPARE(Rooms::inviteLink({code,vector["relay"].toString()}),vector["invite"].toString());
        }
    }
    void randomRoomUniqueness() {
        QSet<QString> codes; for (int i=0;i<1000;++i) codes.insert(Rooms::createCode()); QCOMPARE(codes.size(),1000);
        for (const auto &code : codes) QCOMPARE(Rooms::normalizeCode(code),code);
    }
    void invitationValidation() {
        const auto code=Rooms::createCode();
        for (const auto &bad : {QString(),QString("LCL-123"),QString(26,'I'),QString("<script>")}) QVERIFY_EXCEPTION_THROWN(Rooms::normalizeCode(bad),std::runtime_error);
        for (const auto &bad : {"file:///C:/x","https://example.com","tcp://user:password@host:1","tcp://host:1/path","tcp://host:1?x=y","tcp://host:0"})
            QVERIFY_EXCEPTION_THROWN(Rooms::validateRelay(bad),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(Rooms::parseInvite("localer://user@join/"+code),std::runtime_error);
        QVERIFY_EXCEPTION_THROWN(Rooms::parseInvite("localer://evil/"+code),std::runtime_error);
    }
    void customAndLegacyInvites() {
        Rooms::Room room{Rooms::createCode(),"tcp://example.com:11010"};
        const auto parsed=Rooms::parseInvite(Rooms::inviteLink(room)); QCOMPARE(parsed.code,room.code); QCOMPARE(parsed.relay,room.relay);
        const auto migrated=Rooms::parseInvite("LOCALER://join/"+room.code+"?relay=tcp%3A%2F%2Fpublic.easytier.top%3A11010"); QCOMPARE(migrated.relay,Rooms::defaultRelay);
    }
    void managementAndSecretIsolation() {
        const Rooms::Room room{Rooms::createCode(),Rooms::defaultRelay};
        const auto args=Engine::arguments(room,12345,"Test");
        QVERIFY(args.contains("127.0.0.1:12345")); QVERIFY(args.contains("127.0.0.1/32")); QVERIFY(args.contains("--private-mode"));
        QVERIFY(!args.contains("--enable-exit-node")); QVERIFY(!args.join(' ').contains(Rooms::credentials(room.code).secret)); QVERIFY(!args.contains("--enable-udp-broadcast-relay"));
    }
    void noAdapterNeverConnected() {
        const auto state=Engine::evaluate({},rows(true,true),{},2000); QCOMPARE(state.status,"connecting"); QVERIFY(state.ip.isEmpty()); QVERIFY(state.peers.isEmpty());
    }
    void adapterWithoutReachability() {
        const auto state=Engine::evaluate({},rows(false),{"10.80.1.1"},16000); QCOMPARE(state.status,"degraded"); QVERIFY(!state.relayConnected);
    }
    void waitingThenConnected() {
        auto state=Engine::evaluate({},rows(),{"10.80.1.1"},2000); QCOMPARE(state.status,"waiting"); QCOMPARE(state.peers.size(),1);
        state=Engine::evaluate(state,rows(true,true),{"10.80.1.1"},3000); QCOMPARE(state.status,"connected"); QCOMPARE(state.peers.size(),2); QCOMPARE(state.peers[1].latency,40.5);
    }
    void directConnectionSurvivesRelayLoss() {
        const auto state=Engine::evaluate({},rows(false,true),{"10.80.1.1"},16000); QCOMPARE(state.status,"connected"); QVERIFY(!state.relayConnected);
    }
    void stopIsIdempotent() {
        Engine engine("missing-directory"); engine.stop(); engine.stop(); QCOMPARE(engine.state().status,"idle"); QCOMPARE(engine.processId(),0);
        QVERIFY_EXCEPTION_THROWN(engine.start({Rooms::createCode(),Rooms::defaultRelay},"Test"),std::runtime_error);
        QCOMPARE(engine.processId(),0);
    }
    void missingPacketDependencyIsCaught() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        for (const auto &name : {"easytier-core.exe","easytier-cli.exe","wintun.dll"}) {QFile file(directory.filePath(name));QVERIFY(file.open(QIODevice::WriteOnly));}
        Engine engine(directory.path());
        QVERIFY_EXCEPTION_THROWN(engine.start({Rooms::createCode(),Rooms::defaultRelay},"Test"),std::runtime_error);
        QCOMPARE(engine.processId(),0);
    }
    void failedStartAndCancellation() {
        QTemporaryDir directory; QVERIFY(directory.isValid());
        for (const auto &name : {"easytier-core.exe","easytier-cli.exe","wintun.dll","Packet.dll"}) {QFile file(directory.filePath(name));QVERIFY(file.open(QIODevice::WriteOnly));}
        Engine engine(directory.path()); engine.start({Rooms::createCode(),Rooms::defaultRelay},"Test");
        QTRY_COMPARE_WITH_TIMEOUT(engine.state().status,QString("error"),2000);
        QVERIFY(!engine.state().hasRoom); QCOMPARE(engine.processId(),0);
        engine.start({Rooms::createCode(),Rooms::defaultRelay},"Test"); engine.stop();
        QTest::qWait(30); QCOMPARE(engine.state().status,"idle"); QVERIFY(!engine.state().hasRoom); QCOMPARE(engine.processId(),0);
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "core_tests.moc"
