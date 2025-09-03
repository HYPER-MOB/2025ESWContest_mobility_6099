#include "datacheckwindow.h"
#include "ui_datacheckwindow.h"
#include <QDateTime>

DataCheckWindow::DataCheckWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DataCheckWindow)
{
    ui->setupUi(this);


    ui->progressLine->setRange(0, 0);      
    ui->progressLine->setTextVisible(false);
    ui->loadingLabel->setText(QString());  

    connect(&m_timer, &QTimer::timeout, this, &DataCheckWindow::advance);
}

DataCheckWindow::~DataCheckWindow()
{
    delete ui;
}

void DataCheckWindow::begin(bool hasData)
{
    m_hasData = hasData;
    m_phase   = Phase::Checking;

    setMessage(QStringLiteral("데이터가 있는지 확인합니다..."), true); // busy
    ui->progressLine->setRange(0, 0);
    m_timer.start(1500); // 1.5초 뒤 다음 단계
}

void DataCheckWindow::advance()
{
    switch (m_phase)
    {
    case Phase::Checking:
        if (m_hasData) {
            m_phase = Phase::DataFoundMsg;
            setMessage(QStringLiteral("데이터가 확인되었습니다."), false);
            setDeterminate(0);
            m_timer.start(1000);
        } else {
            m_phase = Phase::NoDataMsg;
            setMessage(QStringLiteral("데이터가 없습니다."), false);
            setDeterminate(0);
            m_timer.start(1200);
        }
        break;

    case Phase::DataFoundMsg:
        m_phase = Phase::Applying;
        setMessage(QStringLiteral("적용중입니다..."), true);
        m_timer.start(1800);
        break;

    case Phase::NoDataMsg:
        m_phase = Phase::Measuring;
        setMessage(QStringLiteral("신체 측정 중입니다..."), true);
        m_timer.start(2200);
        break;

    case Phase::Measuring:
        m_phase = Phase::Calculating;
        setMessage(QStringLiteral("계산중입니다..."), true);
        m_timer.start(1600);
        break;

    case Phase::Calculating:
        m_phase = Phase::Applying;
        setMessage(QStringLiteral("적용중입니다..."), true);
        m_timer.start(1800);
        break;

    case Phase::Applying:
        m_phase = Phase::Done;
        m_timer.stop();
        emit dataCheckFinished();
        break;

    default:
        break;
    }
}

void DataCheckWindow::setMessage(const QString& text, bool busy)
{
    ui->loadingLabel->setText(text);
    if (busy) {
        ui->progressLine->setRange(0, 0);   
    } else {
        ui->progressLine->setRange(0, 100); 
    }
}

void DataCheckWindow::setDeterminate(int v)
{
    if (ui->progressLine->maximum() == 0) {
        ui->progressLine->setRange(0, 100);
    }
    ui->progressLine->setValue(qBound(0, v, 100));
}
