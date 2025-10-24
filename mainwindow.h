#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mainview.h"
#include "datacheckwindow.h"
#include "authwindow.h"
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>
#include <QLabel>
#include <QPauseAnimation>
#include "ipc_client.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class AuthWindow;
class DataCheckWindow;
class MainView;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void onIpcMessage(const IpcMessage& msg);

private:
    Ui::MainWindow *ui;
    AuthWindow* m_auth = nullptr;
    DataCheckWindow* m_data = nullptr;
    MainView* m_main = nullptr;
    void fadeToWidget(QWidget* next, int durationMs = 280);
    // 인트로용 오버레이
    QLabel* m_introLabel = nullptr;
    QWidget* m_introPage = nullptr;
    IpcClient* m_ipc = nullptr;
    bool m_bStart = false;

    void teardownPages();        // 기존 페이지/리소스 정리
    void createIntroIfNeeded();  // 인트로 페이지/라벨 재구성
    void rebuildPages();         // Auth/Data/Main 페이지 재생성 + 시그널 재연결
    void startFromIntro();       // 전체 초기화 후 인트로부터 시작
    void showIntroSequence();  // 인트로 실행
};
#endif // MAINWINDOW_H
