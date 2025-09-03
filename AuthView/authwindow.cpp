#include "authwindow.h"
#include "ui_authwindow.h"

AuthWindow::AuthWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AuthWindow)
{
    ui->setupUi(this);


    ui->progressLine->setRange(0, 0);  
    ui->progressLine->setTextVisible(false);

    ui->loadingLabel->setText(QString());  

    connect(&m_timer, &QTimer::timeout, this, &AuthWindow::advance);
}

AuthWindow::~AuthWindow()
{
    delete ui;
}

void AuthWindow::begin() {
    if (m_started) return;
    m_started = true;

    m_phase = Phase::Loading1;
    setMessage(QStringLiteral("얼굴 인식 중입니다..."), true);  
    ui->progressLine->setRange(0, 0); // busy
    m_timer.start(5000);
}

void AuthWindow::advance() {
    switch (m_phase) {
    case Phase::Loading1:
        m_phase = Phase::RetryNotice;
        setMessage(QStringLiteral("재시도하겠습니다."), false);
        ui->progressLine->setRange(0, 100);
        ui->progressLine->setValue(0);
        m_timer.start(2000);
        break;

    case Phase::RetryNotice:
        m_phase = Phase::Loading2;
        setMessage(QStringLiteral("얼굴 인식 중입니다..."), true);
        ui->progressLine->setRange(0, 0); // 다시 busy
        m_timer.start(5000);
        break;

    case Phase::Loading2:
        m_phase = Phase::SuccessNotice;
        setMessage(QStringLiteral("인증되었습니다."), false);
        ui->progressLine->setRange(0, 100);
        ui->progressLine->setValue(100); 
        m_timer.start(2000);
        break;

    case Phase::SuccessNotice:
        m_phase = Phase::Done;
        m_timer.stop();
        emit authFinished();
        break;
    default:
        break;
    }
}

void AuthWindow::setMessage(const QString& text, bool busy) {
    ui->loadingLabel->setText(text);
    if (busy) {
        ui->progressLine->setRange(0, 0); // busy
    }
}
