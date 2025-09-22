#pragma once
#include <QWidget>
#include <QTimer>
#include "../ipc_client.h"

namespace Ui { class AuthWindow; }

class AuthWindow : public QWidget {
    Q_OBJECT
public:
    explicit AuthWindow(QWidget *parent = nullptr);
    ~AuthWindow();



signals:
    void authFinished();

private slots:
        void begin();
    void advance();
    void onIpcMessage(const IpcMessage& msg);
    void onWaitTimeout();

private:
    enum class Phase { Idle, Loading1, RetryNotice, Loading2, SuccessNotice, Done };
    void setMessage(const QString& text, bool busy);

    Ui::AuthWindow *ui;
    QTimer m_timer;


    IpcClient* m_ipc = nullptr;
    QTimer m_waitTimer;
    QString m_reqId;
    bool m_started = false;
    Phase m_phase = Phase::Idle;
};
