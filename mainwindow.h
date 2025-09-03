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

private:
    Ui::MainWindow *ui;
    AuthWindow* m_auth = nullptr;
    DataCheckWindow* m_data = nullptr;
    MainView* m_main = nullptr;
    void fadeToWidget(QWidget* next, int durationMs = 280);
    // 인트로용 오버레이
    QLabel* m_introLabel = nullptr;
    QWidget* m_introPage = nullptr;

    void showIntroSequence();  // 인트로 실행
};
#endif // MAINWINDOW_H
