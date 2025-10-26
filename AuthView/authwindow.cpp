#include "authwindow.h"
#include "ui_authwindow.h"

static const QString kSock = "/tmp/dcu.demo.sock";

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

static inline QString asJsonCompact(const QJsonObject& o) {
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

AuthWindow::AuthWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AuthWindow)
{
    ui->setupUi(this);

    ui->progressLine->setRange(0, 0);        // 인디터미닛
    ui->progressLine->setTextVisible(false);
    ui->loadingLabel->setText(QString());

    connect(&m_timer, &QTimer::timeout, this, &AuthWindow::advance);

    m_ipc = new IpcClient(kSock, this);
    connect(m_ipc, &IpcClient::messageReceived, this, &AuthWindow::onIpcMessage);

    // m_waitTimer.setSingleShot(true);
    // connect(&m_waitTimer, &QTimer::timeout, this, &AuthWindow::onWaitTimeout);
}

AuthWindow::~AuthWindow() { delete ui; }

void AuthWindow::begin() {
    if (m_started) return;
    m_started = true;

    m_phase = Phase::Loading1;
    setMessage(QStringLiteral("얼굴 인식 중입니다..."), true);
    ui->progressLine->setRange(0, 0);

    QJsonObject payload{
        {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
        {"reason", "face-recognition"},
        {"detail", "start"}
    };

    qInfo().noquote() << "[AUTH] send -> topic=auth/result payload=" << asJsonCompact(payload);

    m_reqId = m_ipc->send("auth/result", payload);

    qInfo().noquote() << "[AUTH] reqId=" << m_reqId;

    // m_waitTimer.start(6000);
}

void AuthWindow::advance() {
    switch (m_phase) {
    case Phase::RetryNotice:
    case Phase::Loading2:
        // 더 이상 사용 안 함
        break;
    default:
        break;
    }
}

void AuthWindow::onIpcMessage(const IpcMessage& msg) {
    if (m_phase == Phase::Fail) {
        qInfo() << "[AUTH] already in FAIL phase. ignore further messages.";
        return;
    }

    qInfo().noquote() << "[AUTH] recv <- topic=" << msg.topic
                      << " reqId=" << msg.reqId
                      << " payload=" << asJsonCompact(msg.payload);

    if (msg.topic == "auth/result") {
        if (!m_reqId.isEmpty() && msg.reqId != m_reqId) {
            qInfo() << "[AUTH] reqId mismatch. ignore.";
            return;
        }

        // if (m_waitTimer.isActive()) m_waitTimer.stop();

        const QJsonObject& p = msg.payload;
        int err = p.value("error").toInt(-999);

        if (err == 0) {
            m_phase = Phase::SuccessNotice;
            setMessage(QStringLiteral("인증되었습니다."), false);
            ui->progressLine->setRange(0, 100);
            ui->progressLine->setValue(100);

            QTimer::singleShot(1200, this, [this, p] {
                m_phase = Phase::Done;
                emit authFinished(p);
            });
        } else {
            m_phase = Phase::Fail;

            if (m_timer.isActive()) m_timer.stop();

            setMessage(QStringLiteral("인증이 실패했습니다."), false);
            ui->progressLine->setRange(0, 100);
            ui->progressLine->setValue(0);   // 0%에서 멈춘 모양으로 고정

        }
        return;
    }

    if (msg.topic == "auth/process") {
        QString text = msg.payload.value("message").toString();
        if (!text.isEmpty()) {
            qInfo().noquote() << "[AUTH] process message=" << text;
            setMessage(text, true);
        } else {
            qInfo() << "[AUTH] process message: empty";
        }
        return;
    }
}

void AuthWindow::onWaitTimeout() {

}

void AuthWindow::setMessage(const QString& text, bool busy) {
    ui->loadingLabel->setText(text);
    if (busy) {
        ui->progressLine->setRange(0, 0);    // 인디터미닛
    } else {
        ui->progressLine->setRange(0, 100);  // 디터미닛
    }
}
