#include "mainwindow.h"
#include <QApplication>
#include <QClipboard>
#include <QFrame>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMimeData>
#include <QScopeGuard>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QtTest>

class UiTests : public QObject {
    Q_OBJECT
private slots:
    void nativeLightInterface() {
        QApplication::setStyle("Fusion");
        auto *savedClipboard=new QMimeData;
        if (const auto *data=QApplication::clipboard()->mimeData()) for (const auto &format : data->formats()) savedClipboard->setData(format,data->data(format));
        auto restoreClipboard=qScopeGuard([savedClipboard] {
            if (QApplication::clipboard()->text()=="10.90.1.1") QApplication::clipboard()->setMimeData(savedClipboard);
            else delete savedClipboard;
        });
        Engine engine("missing-directory"); MainWindow window(&engine,false); window.setAttribute(Qt::WA_DontShowOnScreen); window.show(); QTest::qWait(150);
        auto *tabs=window.findChild<QTabWidget*>("tabs"); QVERIFY(tabs); QCOMPARE(tabs->count(),2);
        tabs->setCurrentIndex(1); QCOMPARE(tabs->currentIndex(),1); tabs->setCurrentIndex(0);
        QTest::mouseClick(window.findChild<QPushButton*>("createRoom"),Qt::LeftButton);
        QVERIFY(window.findChild<QLabel*>("error")->text().contains("yönetici"));
        const QString artifacts=qEnvironmentVariable("LOCALER_ARTIFACT_DIR",QDir::currentPath()+"/artifacts/qt"); QDir().mkpath(artifacts);
        window.applyState({}); QTest::qWait(50); QVERIFY(window.grab().save(artifacts+"/qt-home.png"));
        NetworkState state; state.hasRoom=true; state.room={"LCL-ABCDE-FGHJK-LMNPQ-RSTUV-WXYZ23",Rooms::defaultRelay}; state.status="connected"; state.ip="10.90.1.1"; state.message="1 arkadaş cihazına bağlantı var.";
        state.peers={{"<script>Test</script>","10.90.1.1","Bu cihaz",true,-1},{"Arkadaş-PC","10.90.1.2","Doğrudan",false,12.4}};
        window.applyState(state); QTest::qWait(50);
        auto *peers=window.findChild<QTableWidget*>("peers"); QCOMPARE(peers->rowCount(),2); QVERIFY(peers->item(0,0)->text().startsWith("<script>"));
        QVERIFY(window.grab().save(artifacts+"/qt-room.png"));
        QTest::mouseClick(window.findChild<QPushButton*>("copyIp"),Qt::LeftButton); QCOMPARE(QApplication::clipboard()->text(),state.ip);
        state.room.relay="tcp://example.com:11010"; window.applyState(state); QVERIFY(window.findChild<QPushButton*>("copyCode")->isHidden());
        state.status="degraded"; window.applyState(state); QVERIFY(window.findChild<QLabel*>("headerStatus")->text().contains("ulaşılamıyor"));
        QTest::mouseClick(window.findChild<QPushButton*>("leave"),Qt::LeftButton); QCOMPARE(engine.state().status,"idle");
        QVERIFY(window.findChild<QFrame*>("room")->isHidden());
        QFile report(artifacts+"/ui-smoke.json"); QVERIFY(report.open(QIODevice::WriteOnly));
        report.write(QJsonDocument(QJsonObject{{"passed",true},{"framework","Qt Widgets / C++"},{"version",LOCALER_VERSION},{"tabs",true},{"clipboard",true},{"plainPeerNames",true},{"customRelayInvite",true},{"reset",true}}).toJson());
    }
};
QTEST_MAIN(UiTests)
#include "ui_tests.moc"
