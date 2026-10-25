#pragma once
#include "engine.h"
#include <QMainWindow>
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QFrame;
class QTableWidget;
class QPlainTextEdit;
class QCloseEvent;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(Engine *engine,bool administrator,QWidget *parent=nullptr);
    void applyState(const NetworkState &state);
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    void connectRoom(bool create);
    void showError(const QString &text);
    void copy(const QString &text);
    Engine *m_engine;
    bool m_administrator;
    NetworkState m_current;
    QLabel *m_headerStatus, *m_error, *m_roomStatus, *m_detail, *m_ip, *m_code, *m_inviteHint, *m_peerCount, *m_tip, *m_feedback;
    QFrame *m_adminNotice, *m_setup, *m_roomCard, *m_settings;
    QTabWidget *m_tabs;
    QLineEdit *m_nickname, *m_joinNickname, *m_invite, *m_relay;
    QPushButton *m_createButton, *m_joinButton, *m_copyIp, *m_copyCode, *m_settingsButton, *m_logsButton;
    QTableWidget *m_peers;
    QPlainTextEdit *m_logs;
};
