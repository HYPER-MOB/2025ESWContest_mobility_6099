// MainView.cpp
#include "mainview.h"
#include "ui_mainview.h"

#include <QDateTime>
#include <QTimer>
#include <QButtonGroup>
#include <QStackedWidget>
#include <QToolButton>
#include <QPushButton>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QPauseAnimation>
#include <QEasingCurve>
#include <QJsonObject>
#include <QDebug>
#include <QLocalSocket>
#include <algorithm>

#include "../ipc_client.h"

// 값 범위 상수 정의
namespace {
    const int SEAT_TILT_MIN = 0;
    const int SEAT_TILT_MAX = 90;

    const int SIDE_MIN = -100;
    const int SIDE_MAX = 100;

    const int FOREAFT_MIN = -300;
    const int FOREAFT_MAX = 300;
    const int FOREAFT_STEP = 5;

    const int BACKMIRROR_MIN = -45;
    const int BACKMIRROR_MAX = 45;

    const QString kSock = "/tmp/dcu.demo.sock";
}

MainView::MainView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainView)
{
    ui->setupUi(this);

    // IPC
    m_ipc = new IpcClient(kSock, this);
    connect(m_ipc, &IpcClient::messageReceived, this, &MainView::onIpcMessage);

    // 중앙 스택: 기본은 설정 페이지
    ui->stackedCenter->setCurrentWidget(ui->pageSettings);
    ui->btnMenu->setText(QStringLiteral("주행"));
    connect(ui->btnMenu, &QToolButton::clicked, this, [this]{
        auto stack = ui->stackedCenter;
        const bool isSettings = (stack->currentWidget() == ui->pageSettings);
        const int nextIndex = isSettings
                            ? stack->indexOf(ui->pageDashboard)
                            : stack->indexOf(ui->pageSettings);
        fadeToPage(stack, nextIndex, 320);
    });

    // 상단 탭 버튼 그룹
    m_group = new QButtonGroup(this);
    m_group->setExclusive(true);
    m_group->addButton(ui->btnSeatTilt);
    m_group->addButton(ui->btnSideMirror);
    m_group->addButton(ui->btnSeatForeAft);
    m_group->addButton(ui->btnBackMirror);

    // 기본 탭 및 상세 스택
    ui->btnSeatTilt->setChecked(true);
    ui->stkDetail->setCurrentIndex(0);

    connect(ui->btnSeatTilt,    &QPushButton::clicked, this, [this]{ fadeToPage(ui->stkDetail, 0, 240); });
    connect(ui->btnSideMirror,  &QPushButton::clicked, this, [this]{
        fadeToPage(ui->stkDetail, 1, 240);
        // 사이드미러 페이지 진입 시 기본 Left 선택(라디오가 둘 다 해제된 경우)
        if (!ui->radioLeft->isChecked() && !ui->radioRight->isChecked())
            ui->radioLeft->setChecked(true);
        updateSideMirrorLabel();
    });
    connect(ui->btnSeatForeAft, &QPushButton::clicked, this, [this]{ fadeToPage(ui->stkDetail, 2, 240); });
    connect(ui->btnBackMirror,  &QPushButton::clicked, this, [this]{ fadeToPage(ui->stkDetail, 3, 240); });

    // 사이드미러 라디오 기본값 및 토글 시 표시 갱신
    ui->radioLeft->setChecked(true);
    connect(ui->radioLeft,  &QRadioButton::toggled, this, [this](bool){ updateSideMirrorLabel(); });
    connect(ui->radioRight, &QRadioButton::toggled, this, [this](bool){ updateSideMirrorLabel(); });

    // Up 버튼
    connect(ui->btnUp, &QToolButton::clicked, this, [this]{
        switch (ui->stkDetail->currentIndex()) {
        case 0: // 시트 기울기
            m_seatTiltDeg = std::min(SEAT_TILT_MAX, m_seatTiltDeg + 1);
            ui->lblSeatTiltValue->setText(QString::number(m_seatTiltDeg) + u8"°");
            sendApply(QJsonObject{{"seatTilt", m_seatTiltDeg}});
            break;
        case 1: // 사이드 미러
        {
            const bool left = ui->radioLeft->isChecked();
            if (left) {
                m_sideL = std::min(SIDE_MAX, m_sideL + 1);
                sendApply(QJsonObject{{"sideL", m_sideL}});
            } else {
                m_sideR = std::min(SIDE_MAX, m_sideR + 1);
                sendApply(QJsonObject{{"sideR", m_sideR}});
            }
            updateSideMirrorLabel();
            break;
        }
        case 2: // 시트 앞뒤
            m_foreAft = std::min(FOREAFT_MAX, m_foreAft + FOREAFT_STEP);
            ui->lblSeatForeAftValue->setText(QString("%1 mm").arg(m_foreAft));
            sendApply(QJsonObject{{"foreAft", m_foreAft}});
            break;
        case 3: // 백 미러
            m_backMirrorDeg = std::min(BACKMIRROR_MAX, m_backMirrorDeg + 1);
            ui->lblBackMirrorValue->setText(QString::number(m_backMirrorDeg) + u8"°");
            sendApply(QJsonObject{{"backMirror", m_backMirrorDeg}});
            break;
        }
    });

    // Down 버튼
    connect(ui->btnDown, &QToolButton::clicked, this, [this]{
        switch (ui->stkDetail->currentIndex()) {
        case 0: // 시트 기울기
            m_seatTiltDeg = std::max(SEAT_TILT_MIN, m_seatTiltDeg - 1);
            ui->lblSeatTiltValue->setText(QString::number(m_seatTiltDeg) + u8"°");
            sendApply(QJsonObject{{"seatTilt", m_seatTiltDeg}});
            break;
        case 1: // 사이드 미러
        {
            const bool left = ui->radioLeft->isChecked();
            if (left) {
                m_sideL = std::max(SIDE_MIN, m_sideL - 1);
                sendApply(QJsonObject{{"sideL", m_sideL}});
            } else {
                m_sideR = std::max(SIDE_MIN, m_sideR - 1);
                sendApply(QJsonObject{{"sideR", m_sideR}});
            }
            updateSideMirrorLabel();
            break;
        }
        case 2: // 시트 앞뒤
            m_foreAft = std::max(FOREAFT_MIN, m_foreAft - FOREAFT_STEP);
            ui->lblSeatForeAftValue->setText(QString("%1 mm").arg(m_foreAft));
            sendApply(QJsonObject{{"foreAft", m_foreAft}});
            break;
        case 3: // 백 미러
            m_backMirrorDeg = std::max(BACKMIRROR_MIN, m_backMirrorDeg - 1);
            ui->lblBackMirrorValue->setText(QString::number(m_backMirrorDeg) + u8"°");
            sendApply(QJsonObject{{"backMirror", m_backMirrorDeg}});
            break;
        }
    });

    // 시계
    updateClock();
    m_clockTimer = new QTimer(this);
    m_clockTimer->setInterval(1000);
    connect(m_clockTimer, &QTimer::timeout, this, &MainView::updateClock);
    m_clockTimer->start();

    // 초기 표시 동기화
    updateSideMirrorLabel();
    ui->lblSeatTiltValue->setText(QString::number(m_seatTiltDeg) + u8"°");
    ui->lblSeatForeAftValue->setText(QString("%1 mm").arg(m_foreAft));
    ui->lblBackMirrorValue->setText(QString::number(m_backMirrorDeg) + u8"°");
}

MainView::~MainView()
{
    delete ui;
}

void MainView::on_pushButton_clicked()
{
    // (기존 테스트용) 필요 없으면 삭제 가능
    QLocalSocket sock;
    sock.connectToServer(kSock);
    if (!sock.waitForConnected(1000)) {
        qDebug() << "connect failed:" << sock.errorString();
        return;
    }
    sock.write("hello");
    sock.flush();
    sock.waitForBytesWritten(1000);
    sock.disconnectFromServer();
}

void MainView::updateClock()
{
    ui->lblClock->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd (ddd) HH:mm:ss"));
}

void MainView::fadeToPage(QStackedWidget* stack, int nextIndex, int durationMs)
{
    if (!stack) return;
    if (nextIndex == stack->currentIndex()) return;

    QWidget* cur  = stack->currentWidget();
    QWidget* next = stack->widget(nextIndex);
    if (!next || next == cur) return;

    if (stack->property("isAnimating").toBool()) return;
    stack->setProperty("isAnimating", true);

    auto* outEff = new QGraphicsOpacityEffect(cur);
    auto* inEff  = new QGraphicsOpacityEffect(next);
    cur->setGraphicsEffect(outEff);
    next->setGraphicsEffect(inEff);
    inEff->setOpacity(0.0);

    auto* fadeOut = new QPropertyAnimation(outEff, "opacity", this);
    fadeOut->setDuration(durationMs);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InOutQuad);

    auto* fadeIn = new QPropertyAnimation(inEff, "opacity", this);
    fadeIn->setDuration(durationMs);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);

    auto* seq = new QSequentialAnimationGroup(this);
    stack->setEnabled(false);

    // 전환 타이밍: fadeOut 끝나는 시점에 인덱스 교체
    connect(fadeOut, &QPropertyAnimation::finished, this, [this, stack, next](){
        stack->setCurrentWidget(next);
        next->update();

        if (stack == ui->stackedCenter) {
            if (stack->currentWidget() == ui->pageSettings)
                ui->btnMenu->setText(QStringLiteral("주행"));
            else
                ui->btnMenu->setText(QStringLiteral("정지"));
        }
    });

    // 종료 정리
    connect(seq, &QSequentialAnimationGroup::finished, this, [=](){
        if (cur->graphicsEffect())  cur->setGraphicsEffect(nullptr);
        if (next->graphicsEffect()) next->setGraphicsEffect(nullptr);
        stack->setEnabled(true);
        stack->setProperty("isAnimating", false);
    });

    seq->addAnimation(fadeOut);
    seq->addAnimation(fadeIn);
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainView::updateSideMirrorLabel()
{
    ui->lblSideMirrorValue->setText(
        QString("L: %1 / R: %2").arg(m_sideL).arg(m_sideR));
}

void MainView::sendApply(const QJsonObject& changes)
{
    QJsonObject payload = changes;
    payload.insert("ts", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    m_powerReqId = m_ipc->send("power/apply", payload);
}

void MainView::onIpcMessage(const IpcMessage& msg)
{
    if (msg.topic == "power/apply/ack" && (!m_powerReqId.isEmpty() && msg.reqId == m_powerReqId)) {
        const bool ok = msg.payload.value("ok").toBool(false);
        qInfo() << "[power/apply/ack]" << ok << msg.payload;
    }
}
