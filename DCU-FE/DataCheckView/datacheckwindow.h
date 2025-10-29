#pragma once
#include <QWidget>
#include <QTimer>
#include <QJsonObject>
#include "../ipc_client.h"

namespace Ui { class DataCheckWindow; }

class DataCheckWindow : public QWidget {
    Q_OBJECT
public:
    explicit DataCheckWindow(QWidget *parent = nullptr);
    ~DataCheckWindow();


signals:
    void dataCheckFinished();
    void profileResolved(const QJsonObject& profile);

private slots:
    void begin(bool hasData );
    void beginWithAuth(const QJsonObject& authPayload);
    void advance();
    void onIpcMessage(const IpcMessage& msg);
    void onWaitTimeout();


private:
    enum class Phase { Idle, Checking, DataFoundMsg, NoDataMsg, Measuring, Calculating, Applying, Done };
    void setMessage(const QString& text, bool busy);
    void setDeterminate(int v);

    Ui::DataCheckWindow *ui;
    QTimer m_timer;

    IpcClient* m_ipc = nullptr;
    QTimer m_waitTimer;
    QString m_dataReqId;
    QString m_powerReqId;
    QJsonObject m_dataPayload;
    QJsonObject m_authPayload; 

    bool m_hasData = false;
    Phase m_phase = Phase::Idle;
};
