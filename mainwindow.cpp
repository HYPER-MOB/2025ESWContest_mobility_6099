// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "authwindow.h"
#include "datacheckwindow.h"
#include "mainview.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QPauseAnimation>
#include <QEasingCurve>
#include <QLabel>
#include <QMetaObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 창 크기 고정 (1024x600)
    setFixedSize(1024, 600);

    // QMainWindow 고유 요소 숨김
    if (menuBar()) menuBar()->hide();
    if (statusBar()) statusBar()->hide();

    // 프레임/타이틀바 제거
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);


    m_introPage = new QWidget(this);
    m_introPage->setObjectName("pageIntro");
    m_introPage->setStyleSheet("QWidget#pageIntro { background: #FFFFFF; }");
    ui->stack->insertWidget(0, m_introPage);
    ui->stack->setCurrentWidget(m_introPage);

    // 중앙 텍스트 라벨
    m_introLabel = new QLabel(m_introPage);
    m_introLabel->setObjectName("introLabel");
    m_introLabel->setAlignment(Qt::AlignCenter);
    m_introLabel->setStyleSheet("color:#000000; font-size:42px; font-weight:800;");
    m_introLabel->setGeometry(0, 0, 1024, 600);

    m_auth = new AuthWindow(this);
    m_data = new DataCheckWindow(this);
    m_main = new MainView(this);

    ui->stack->insertWidget(1, m_auth);
    ui->stack->insertWidget(2, m_data);
    ui->stack->insertWidget(3, m_main);

    // 인트로 애니메이션 시작
    showIntroSequence();

    // Auth → DataCheck
    connect(m_auth, &AuthWindow::authFinished, this, [this]{
        bool hasData = false; // later
        fadeToWidget(m_data);
        QMetaObject::invokeMethod(m_data, "begin", Qt::QueuedConnection,
                                  Q_ARG(bool, hasData));
    });

    // DataCheck → Main
    connect(m_data, &DataCheckWindow::dataCheckFinished, this, [this]{
        fadeToWidget(m_main, 320);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fadeToWidget(QWidget* next, int durationMs)
{
    QWidget* cur = ui->stack->currentWidget();
    if (!next || next == cur) return;

    auto *outEff = new QGraphicsOpacityEffect(cur);
    auto *inEff  = new QGraphicsOpacityEffect(next);
    cur->setGraphicsEffect(outEff);
    next->setGraphicsEffect(inEff);
    inEff->setOpacity(0.0);

    auto *fadeOut = new QPropertyAnimation(outEff, "opacity");
    fadeOut->setDuration(durationMs);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InOutQuad);

    auto *fadeIn = new QPropertyAnimation(inEff, "opacity");
    fadeIn->setDuration(durationMs);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);

    auto *seq = new QSequentialAnimationGroup(this);
    ui->stack->setEnabled(false);

    connect(fadeOut, &QPropertyAnimation::finished, this, [this, next](){
        ui->stack->setCurrentWidget(next);
        next->update();
    });

    connect(seq, &QSequentialAnimationGroup::finished, this, [cur, next, this](){
        if (cur->graphicsEffect())  cur->setGraphicsEffect(nullptr);
        if (next->graphicsEffect()) next->setGraphicsEffect(nullptr);
        ui->stack->setEnabled(true);
    });

    seq->addAnimation(fadeOut);
    seq->addAnimation(fadeIn);
    seq->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::showIntroSequence()
{
    auto *eff = new QGraphicsOpacityEffect(m_introLabel);
    eff->setOpacity(0.0);
    m_introLabel->setGraphicsEffect(eff);

    auto *seq = new QSequentialAnimationGroup(this);

    auto makeFadeIn  = [&](int ms){ auto a = new QPropertyAnimation(eff, "opacity", this);
                                    a->setDuration(ms); a->setStartValue(0.0); a->setEndValue(1.0);
                                    a->setEasingCurve(QEasingCurve::InOutQuad); return a; };
    auto makeFadeOut = [&](int ms){ auto a = new QPropertyAnimation(eff, "opacity", this);
                                    a->setDuration(ms); a->setStartValue(1.0); a->setEndValue(0.0);
                                    a->setEasingCurve(QEasingCurve::InOutQuad); return a; };

 
    m_introLabel->setText(u8"반갑습니다");
    seq->addAnimation(makeFadeIn(500));
    seq->addAnimation(new QPauseAnimation(1000, this));
    seq->addAnimation(makeFadeOut(500));


    connect(seq, &QSequentialAnimationGroup::currentAnimationChanged, this,
        [=](QAbstractAnimation*){
            if (m_introLabel->text() == u8"반갑습니다" && eff->opacity() == 0.0) {
                m_introLabel->setText(u8"얼굴 인식을 시작합니다");
            }
        });

    seq->addAnimation(makeFadeIn(500));
    seq->addAnimation(new QPauseAnimation(1000, this));
    seq->addAnimation(makeFadeOut(500));

    connect(seq, &QSequentialAnimationGroup::finished, this, [=]{
        if (m_introLabel) {
            m_introLabel->hide();
            m_introLabel->deleteLater();
            m_introLabel = nullptr;
        }
        fadeToWidget(m_auth, 320);
        QMetaObject::invokeMethod(m_auth, "begin", Qt::QueuedConnection);
    });

    seq->start(QAbstractAnimation::DeleteWhenStopped);
}
