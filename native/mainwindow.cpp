#include "mainwindow.h"
#include "platform.h"
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QFrame>
#include <QHeaderView>
#include <QHostInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
QLabel *label(const QString &text,const QString &name={},QWidget *parent=nullptr) {
    auto *item=new QLabel(text,parent); item->setTextFormat(Qt::PlainText); item->setWordWrap(true);
    if (!name.isEmpty()) item->setObjectName(name);
    return item;
}
QPushButton *button(const QString &text,const QString &name={},bool primary=false) {
    auto *item=new QPushButton(text); item->setObjectName(name); item->setCursor(Qt::PointingHandCursor);
    item->setProperty("primary",primary); item->setMinimumHeight(primary?40:32); return item;
}
QLineEdit *input(const QString &name,const QString &placeholder,int maxLength) {
    auto *item=new QLineEdit; item->setObjectName(name); item->setPlaceholderText(placeholder); item->setMaxLength(maxLength); item->setMinimumHeight(40); return item;
}
QFrame *frame(const QString &name) { auto *item=new QFrame; item->setObjectName(name); return item; }
void field(QVBoxLayout *layout,const QString &text,QLineEdit *edit) {
    auto *title=label(text,"fieldLabel"); title->setBuddy(edit); layout->addWidget(title); layout->addWidget(edit); layout->addSpacing(12);
    edit->setAccessibleName(text);
}
}

MainWindow::MainWindow(Engine *engine,bool administrator,QWidget *parent):QMainWindow(parent),m_engine(engine),m_administrator(administrator) {
    Q_INIT_RESOURCE(resources);
    setWindowTitle("Localer"); setWindowIcon(QIcon(":/localer.png")); resize(780,850); setMinimumSize(580,650);
    setStyleSheet(R"(
QMainWindow,QScrollArea,QWidget#page{background:#f6f7f9;color:#202731;font-family:'Segoe UI';font-size:13px;}
QFrame#header{background:white;border-bottom:1px solid #e8ebef;} QLabel{background:transparent;color:#38465a;}
QLabel#brand{font-size:19px;font-weight:600;color:#202731;} QLabel#heading{font-size:26px;font-weight:600;color:#202731;}
QLabel#muted,QLabel#detail{color:#687384;font-size:12px;} QLabel#fieldLabel{color:#485365;font-size:12px;}
QFrame#setup,QFrame#room{background:white;border:1px solid #e2e6ec;border-radius:12px;}
QFrame#admin{background:#fffbf3;border:1px solid #eadab9;border-radius:8px;} QLabel#adminText{color:#846b3d;font-size:12px;}
QLabel#error{color:#a34444;background:#fff4f4;border:1px solid #f0d1d1;border-radius:7px;padding:12px;}
QLineEdit{background:white;color:#242e3c;border:1px solid #d8dee7;border-radius:6px;padding:8px 11px;selection-background-color:#2860ce;}
QLineEdit:focus{border-color:#5582d8;} QPushButton{background:white;color:#47566c;border:1px solid #d8dee7;border-radius:6px;padding:6px 12px;}
QPushButton:hover{background:#f4f6f9;} QPushButton:disabled{color:#a6afbd;background:#f5f6f8;}
QPushButton[primary="true"]{background:#2860ce;color:white;border-color:#2860ce;font-weight:500;}
QPushButton[primary="true"]:hover{background:#2055bc;} QPushButton[primary="true"]:disabled{background:#9db7e8;border-color:#9db7e8;}
QPushButton#leave{border:none;color:#a65050;background:transparent;} QPushButton#copyInvite{border:none;color:#2860ce;}
QPushButton[disclosure="true"]{background:transparent;border:none;border-bottom:1px solid #e3e7ed;border-radius:0;color:#69768a;text-align:left;padding:15px 2px;}
QTabWidget::pane{border:0;background:white;} QTabBar::tab{background:white;color:#738092;padding:17px 0px;margin:0px 14px;border-bottom:2px solid transparent;}
QTabBar::tab:selected{color:#2860ce;border-bottom-color:#2860ce;} QTabBar{background:white;}
QLabel#title{font-size:18px;font-weight:600;color:#202731;} QFrame#ipBox{background:#f5f8fe;border:1px solid #e5ecf8;border-radius:8px;}
QLabel#ip{font-family:Consolas;font-size:25px;font-weight:600;color:#2c4e80;} QLabel#code{font-family:Consolas;font-size:13px;color:#3c495d;}
QLabel#hint,QLabel#tip{font-size:11px;color:#7b8797;} QLabel#tip{background:#fafbfc;padding:10px;border-radius:6px;}
QTableWidget{border:0;gridline-color:#eff1f5;background:white;color:#3f4c60;font-size:12px;selection-background-color:#edf3ff;selection-color:#244b83;}
QHeaderView::section{background:white;color:#7c899b;border:0;border-bottom:1px solid #e6eaf0;padding:8px 3px;text-align:left;font-size:11px;}
QPlainTextEdit{background:white;border:1px solid #e2e6ec;border-radius:6px;color:#65748b;font-family:Consolas;font-size:11px;padding:8px;}
QLabel#footer{color:#9aa3af;font-size:10px;} QLabel#feedback{background:#26364d;color:white;border-radius:6px;padding:8px;}
QScrollBar:vertical{background:#f6f7f9;width:9px;} QScrollBar::handle:vertical{background:#c9d0d9;border-radius:4px;min-height:30px;}
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}
)");
    auto *root=new QWidget; auto *rootLayout=new QVBoxLayout(root); rootLayout->setContentsMargins(0,0,0,0); rootLayout->setSpacing(0);
    auto *header=frame("header"); header->setFixedHeight(66); auto *headerLayout=new QHBoxLayout(header); headerLayout->setContentsMargins(28,0,28,0);
    auto *icon=new QLabel; icon->setPixmap(QPixmap(":/localer.png").scaled(28,28,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    headerLayout->addWidget(icon); headerLayout->addWidget(label("Localer","brand")); headerLayout->addStretch();
    m_headerStatus=label("●  Bağlı değil","headerStatus"); headerLayout->addWidget(m_headerStatus); rootLayout->addWidget(header);
    auto *scroll=new QScrollArea; scroll->setFrameShape(QFrame::NoFrame); scroll->setWidgetResizable(true);
    auto *page=new QWidget; page->setObjectName("page"); auto *horizontal=new QHBoxLayout(page); horizontal->setContentsMargins(24,36,24,24); horizontal->addStretch();
    auto *content=new QWidget; content->setMaximumWidth(580); content->setMinimumWidth(490); auto *layout=new QVBoxLayout(content); layout->setContentsMargins(0,0,0,0); layout->setSpacing(15);
    layout->addWidget(label("Arkadaşlarınla aynı ağda.","heading"));
    layout->addWidget(label("Bir oda oluştur veya davet koduyla arkadaşlarına katıl.","muted")); layout->addSpacing(6);
    m_adminNotice=frame("admin"); auto *adminLayout=new QHBoxLayout(m_adminNotice); adminLayout->setContentsMargins(14,10,14,10);
    adminLayout->addWidget(label("Bağlantı kurmak için yönetici izni gerekiyor.","adminText"),1);
    auto *elevate=button("Yönetici olarak aç","elevate"); adminLayout->addWidget(elevate); layout->addWidget(m_adminNotice); m_adminNotice->setVisible(!administrator);
    connect(elevate,&QPushButton::clicked,this,[this] {
        if (Platform::restartElevated(QCoreApplication::applicationFilePath())) QCoreApplication::quit();
        else showError("Windows yönetici izni verilmedi. Uygulamayı sağ tıklayıp yönetici olarak açabilirsin.");
    });
    m_error=label({},"error"); layout->addWidget(m_error); m_error->hide();
    m_setup=frame("setup"); auto *setupLayout=new QVBoxLayout(m_setup); setupLayout->setContentsMargins(12,0,12,10);
    m_tabs=new QTabWidget; m_tabs->setObjectName("tabs"); setupLayout->addWidget(m_tabs);
    auto *createPage=new QWidget; auto *createLayout=new QVBoxLayout(createPage); createLayout->setContentsMargins(16,22,16,16);
    createLayout->addWidget(label("Yeni bir oda aç","title")); createLayout->addWidget(label("Oluşan davet kodunu arkadaşlarınla paylaş.","muted")); createLayout->addSpacing(16);
    m_nickname=input("nickname","Örn. Arda-PC",32); field(createLayout,"Cihaz adın",m_nickname);
    m_createButton=button("Oda oluştur","createRoom",true); createLayout->addWidget(m_createButton); m_tabs->addTab(createPage,"Oda oluştur");
    auto *joinPage=new QWidget; auto *joinLayout=new QVBoxLayout(joinPage); joinLayout->setContentsMargins(16,22,16,16);
    joinLayout->addWidget(label("Arkadaşlarına katıl","title")); joinLayout->addWidget(label("Aldığın kodu veya davet bağlantısını yapıştır.","muted")); joinLayout->addSpacing(16);
    m_invite=input("invite","LCL-…",1024); field(joinLayout,"Davet kodu veya bağlantısı",m_invite);
    m_joinNickname=input("joinNickname","Örn. Arda-PC",32); field(joinLayout,"Cihaz adın",m_joinNickname);
    m_joinButton=button("Odaya katıl","joinRoom",true); joinLayout->addWidget(m_joinButton); m_tabs->addTab(joinPage,"Odaya katıl"); layout->addWidget(m_setup);
    m_nickname->setText(QHostInfo::localHostName().left(32)); m_joinNickname->setText(m_nickname->text());
    connect(m_nickname,&QLineEdit::textEdited,m_joinNickname,&QLineEdit::setText);
    connect(m_joinNickname,&QLineEdit::textEdited,m_nickname,&QLineEdit::setText);
    connect(m_createButton,&QPushButton::clicked,this,[this]{ connectRoom(true); });
    connect(m_nickname,&QLineEdit::returnPressed,this,[this]{ connectRoom(true); });
    connect(m_joinButton,&QPushButton::clicked,this,[this]{ connectRoom(false); });
    connect(m_invite,&QLineEdit::returnPressed,this,[this]{ connectRoom(false); });
    connect(m_joinNickname,&QLineEdit::returnPressed,this,[this]{ connectRoom(false); });
    m_roomCard=frame("room"); auto *roomLayout=new QVBoxLayout(m_roomCard); roomLayout->setContentsMargins(25,23,25,23); roomLayout->setSpacing(12);
    auto *roomHeading=new QHBoxLayout; roomHeading->addWidget(label("Arkadaş odası","title")); roomHeading->addStretch();
    auto *leave=button("Odadan ayrıl","leave"); roomHeading->addWidget(leave); roomLayout->addLayout(roomHeading);
    connect(leave,&QPushButton::clicked,engine,&Engine::stop);
    m_roomStatus=label({},"roomStatus"); roomLayout->addWidget(m_roomStatus);
    m_detail=label({},"detail"); roomLayout->addWidget(m_detail); roomLayout->addSpacing(3);
    auto *ipBox=frame("ipBox"); auto *ipLayout=new QHBoxLayout(ipBox); ipLayout->setContentsMargins(16,16,16,16);
    auto *ipColumn=new QVBoxLayout; ipColumn->addWidget(label("Sanal IP adresin","hint")); m_ip=label({},"ip"); m_ip->setTextInteractionFlags(Qt::TextSelectableByMouse); ipColumn->addWidget(m_ip); ipLayout->addLayout(ipColumn,1);
    m_copyIp=button("IP’yi kopyala","copyIp"); ipLayout->addWidget(m_copyIp); roomLayout->addWidget(ipBox);
    roomLayout->addSpacing(5); roomLayout->addWidget(label("Davet kodu","hint")); m_code=label({},"code"); m_code->setTextInteractionFlags(Qt::TextSelectableByMouse); roomLayout->addWidget(m_code);
    auto *inviteActions=new QHBoxLayout; m_copyCode=button("Kodu kopyala","copyCode"); inviteActions->addWidget(m_copyCode);
    auto *copyInvite=button("Davet bağlantısını kopyala","copyInvite"); inviteActions->addWidget(copyInvite); inviteActions->addStretch(); roomLayout->addLayout(inviteActions);
    m_inviteHint=label({},"hint"); roomLayout->addWidget(m_inviteHint); roomLayout->addSpacing(4);
    connect(m_copyIp,&QPushButton::clicked,this,[this]{ copy(m_current.ip); });
    connect(m_copyCode,&QPushButton::clicked,this,[this]{ copy(m_current.room.code); });
    connect(copyInvite,&QPushButton::clicked,this,[this]{ if (m_current.hasRoom) copy(Rooms::inviteLink(m_current.room)); });
    m_peerCount=label("Cihazlar","fieldLabel"); roomLayout->addWidget(m_peerCount);
    m_peers=new QTableWidget(0,4); m_peers->setObjectName("peers"); m_peers->setHorizontalHeaderLabels({"Cihaz","Sanal IP","Bağlantı","Gecikme"});
    m_peers->verticalHeader()->hide(); m_peers->setEditTriggers(QAbstractItemView::NoEditTriggers); m_peers->setShowGrid(false);
    m_peers->setSelectionBehavior(QAbstractItemView::SelectRows); m_peers->setSelectionMode(QAbstractItemView::SingleSelection);
    m_peers->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    for (int column=1;column<4;++column) m_peers->horizontalHeader()->setSectionResizeMode(column,QHeaderView::ResizeToContents);
    roomLayout->addWidget(m_peers); m_tip=label({},"tip"); roomLayout->addWidget(m_tip); layout->addWidget(m_roomCard); m_roomCard->hide();
    m_settingsButton=button("Bağlantı ayarları  +","settingsToggle"); m_settingsButton->setProperty("disclosure",true); m_settingsButton->setCheckable(true); layout->addWidget(m_settingsButton);
    m_settings=frame("settings"); auto *settingsLayout=new QVBoxLayout(m_settings); settingsLayout->setContentsMargins(0,0,0,5);
    m_relay=input("relay","tcp://adres:11010",256); m_relay->setText(Rooms::defaultRelay); field(settingsLayout,"Buluşma sunucusu",m_relay);
    settingsLayout->addWidget(label("Topluluk sunucusunu veya kendi sunucunu kullanabilirsin. Özel sunucuda arkadaşlarına tam davet bağlantısını gönder.","hint")); layout->addWidget(m_settings); m_settings->hide();
    connect(m_settingsButton,&QPushButton::toggled,m_settings,&QWidget::setVisible);
    auto *helpButton=button("Nasıl bağlanırım?  +","helpToggle"); helpButton->setProperty("disclosure",true); helpButton->setCheckable(true); layout->addWidget(helpButton);
    auto *help=label("Oyun: Sunucuyu açan kişinin sanal IP adresini ve oyun portunu doğrudan bağlantı alanına yaz.\n\nYerel web servisi: Servisi 0.0.0.0 üzerinde dinlet. Arkadaşların http://sanal-IP:port adresini açsın.\n\nCihaz görünüyor ama servis açılmıyorsa: Windows güvenlik duvarında servisi çalıştıran uygulamaya veya gereken TCP/UDP portuna gelen bağlantı izni ver. İzni odanın sanal ağıyla sınırla; güvenlik duvarını kapatma.\n\nKodu bilenler odaya girebilir. Kişi çıkarma bu sürümde yok; gerekirse yeni oda oluştur. Otomatik LAN keşfi her oyunda çalışmayabilir. İnternet çıkışınız ortak olmaz.","detail"); layout->addWidget(help); help->hide(); connect(helpButton,&QPushButton::toggled,help,&QWidget::setVisible);
    m_logsButton=button("Bağlantı günlüğü  +","logsToggle"); m_logsButton->setProperty("disclosure",true); m_logsButton->setCheckable(true); layout->addWidget(m_logsButton);
    m_logs=new QPlainTextEdit; m_logs->setReadOnly(true); m_logs->setFixedHeight(160); layout->addWidget(m_logs); m_logs->hide(); m_logsButton->hide(); connect(m_logsButton,&QPushButton::toggled,m_logs,&QWidget::setVisible);
    m_feedback=label({},"feedback"); layout->addWidget(m_feedback); m_feedback->hide();
    layout->addWidget(label(QString("Localer %1  ·  Qt %2  ·  Windows").arg(LOCALER_VERSION,QT_VERSION_STR),"footer")); layout->addStretch();
    horizontal->addWidget(content,1); horizontal->addStretch(); scroll->setWidget(page); rootLayout->addWidget(scroll); setCentralWidget(root);
    connect(engine,&Engine::changed,this,[this]{ applyState(m_engine->state()); }); applyState(engine->state());
}

void MainWindow::connectRoom(bool create) {
    showError({});
    if (!m_administrator) { showError("Sanal ağ için yönetici izni gerekiyor. Yönetici olarak aç düğmesini kullan."); return; }
    try {
        const auto room=create?Rooms::Room{Rooms::createCode(),Rooms::validateRelay(m_relay->text())}:Rooms::parseInvite(m_invite->text(),m_relay->text());
        m_engine->start(room,create?m_nickname->text():m_joinNickname->text());
    } catch (const std::exception &error) { showError(QString::fromUtf8(error.what())); }
}
void MainWindow::showError(const QString &text) { m_error->setText(text); m_error->setVisible(!text.isEmpty()); }
void MainWindow::copy(const QString &text) {
    if (text.isEmpty()) return;
    QApplication::clipboard()->setText(text); m_feedback->setText("Panoya kopyalandı"); m_feedback->show(); QTimer::singleShot(2400,m_feedback,&QWidget::hide);
}
void MainWindow::applyState(const NetworkState &state) {
    m_current=state;
    m_setup->setVisible(!state.hasRoom); m_roomCard->setVisible(state.hasRoom);
    m_settingsButton->setVisible(!state.hasRoom); m_settings->setVisible(!state.hasRoom && m_settingsButton->isChecked());
    QString title="Bağlı değil";
    if (state.status=="connecting") title="Bağlanıyor…";
    else if (state.status=="waiting") title="Arkadaşların bekleniyor";
    else if (state.status=="connected") title="Bağlandı";
    else if (state.status=="degraded") title="Sunucuya ulaşılamıyor";
    else if (state.status=="error") title="Bağlantı hatası";
    const QString color=(state.status=="connected" || state.status=="waiting")?"#28805b":(state.status=="connecting" || state.status=="degraded")?"#ab7b22":"#8792a3";
    for (auto *status : {m_headerStatus,m_roomStatus}) { status->setText("●  "+title); status->setStyleSheet("color:"+color+";font-size:12px;"); }
    m_detail->setText(state.message); m_ip->setText(state.ip.isEmpty()?"Hazırlanıyor…":state.ip); m_copyIp->setEnabled(!state.ip.isEmpty());
    m_code->setText(state.room.code); const bool custom=state.room.relay!=Rooms::defaultRelay;
    m_copyCode->setVisible(!custom); m_inviteHint->setText(custom?"Özel sunucu kullanılıyor. Arkadaşlarına tam davet bağlantısını gönder.":"Bu kodu yalnızca güvendiğin kişilerle paylaş.");
    m_peerCount->setText(QString("Cihazlar · %1").arg(state.peers.size())); m_peers->setRowCount(static_cast<int>(state.peers.size()));
    int row=0;
    for (const auto &peer : state.peers) {
        const QStringList values={peer.name+(peer.local?" · Sen":""),peer.ip,peer.connection,peer.latency<0?"—":QString::number(peer.latency,'f',1)+" ms"};
        for (int column=0;column<4;++column) m_peers->setItem(row,column,new QTableWidgetItem(values.at(column)));
        m_peers->setRowHeight(row,42); row++;
    }
    m_peers->setFixedHeight(qMin(6,row)*42+38);
    m_tip->setText(state.ip.isEmpty()?"IP adresin göründüğünde oyun veya servisine bu adresle bağlanabilirsiniz.":QString("Örnek: http://%1:5173 · Servisini 0.0.0.0 üzerinde dinlet ve gereken porta izin ver.").arg(state.ip));
    m_logsButton->setVisible(state.hasRoom || !state.logs.isEmpty()); m_logs->setVisible(m_logsButton->isVisible() && m_logsButton->isChecked()); m_logs->setPlainText(state.logs.join('\n'));
    showError(state.status=="error"?state.error:QString());
}
void MainWindow::closeEvent(QCloseEvent *event) { m_engine->stop(); event->accept(); }
