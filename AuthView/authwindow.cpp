#include "authwindow.h"
#include "ui_authwindow.h"

static const QString kSock = "/tmp/dcu.demo.sock";

AuthWindow::AuthWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AuthWindow)
{
    ui->setupUi(this);

    ui->progressLine->setRange(0, 0);
    ui->progressLine->setTextVisible(false);
    ui->loadingLabel->setText(QString());

    connect(&m_timer, &QTimer::timeout, this, &AuthWindow::advance);

    m_ipc = new IpcClient(kSock, this);
    connect(m_ipc, &IpcClient::messageReceived, this, &AuthWindow::onIpcMessage);

    m_waitTimer.setSingleShot(true);
    connect(&m_waitTimer, &QTimer::timeout, this, &AuthWindow::onWaitTimeout);
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
    m_reqId = m_ipc->send("auth/result", payload);

    m_waitTimer.start(6000);
}

void AuthWindow::advance() {
    switch (m_phase) {
    case Phase::RetryNotice:
        m_phase = Phase::Loading2;
        setMessage(QStringLiteral("얼굴 인식 중입니다..."), true);
        ui->progressLine->setRange(0, 0);

        m_reqId = m_ipc->send("auth/result", QJsonObject{
            {"ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
            {"reason", "face-recognition"},
            {"detail", "retry"}
        });
        m_waitTimer.start(6000);
        break;

    default:
        break;
    }
}

void AuthWindow::onIpcMessage(const IpcMessage& msg) {
    if (msg.topic != "auth/result") return;
    if (!m_reqId.isEmpty() && msg.reqId != m_reqId) return;

    if (m_waitTimer.isActive()) m_waitTimer.stop();

    const QJsonObject& p = msg.payload;
    int err = p.value("error").toInt(-999);
    if (err == 0) {
        m_phase = Phase::SuccessNotice;
        setMessage(QStringLiteral("인증되었습니다."), false);
        ui->progressLine->setRange(0, 100);
        ui->progressLine->setValue(100);

        QTimer::singleShot(2000, this, [this]{
            m_phase = Phase::Done;
            emit authFinished();
        });
    } else {
        m_phase = Phase::RetryNotice;
        setMessage(QStringLiteral("재시도하겠습니다."), false);
        ui->progressLine->setRange(0, 100);
        ui->progressLine->setValue(0);

        m_timer.start(2000);
    }
}

void AuthWindow::onWaitTimeout() {
    m_phase = Phase::RetryNotice;
    setMessage(QStringLiteral("네트워크 지연으로 재시도하겠습니다."), false);
    ui->progressLine->setRange(0, 100);
    ui->progressLine->setValue(0);

    m_timer.start(2000);
}

void AuthWindow::setMessage(const QString& text, bool busy) {
    ui->loadingLabel->setText(text);
    if (busy) ui->progressLine->setRange(0, 0);
}
