#ifndef AUTHWINDOW_H
#define AUTHWINDOW_H

#include <QWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class AuthWindow; }
QT_END_NAMESPACE

class AuthWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AuthWindow(QWidget *parent = nullptr);
    ~AuthWindow();

public slots:
    void begin();  

signals:
    void authFinished();  

private slots:
    void advance();       

private:
    enum class Phase {
        Idle,
        Loading1,      
        RetryNotice,    
        Loading2,      
        SuccessNotice,  
        Done
    };

    void setMessage(const QString &text, bool busy);

private:
    Ui::AuthWindow *ui;
    QTimer m_timer;
    Phase  m_phase = Phase::Idle;
    bool   m_started = false;
};

#endif // AUTHWINDOW_H
