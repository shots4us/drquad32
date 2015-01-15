#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Connection.h"

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QTime>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    Connection connection;

signals:
    void  deviceChanged();

private:
    Ui::MainWindow *ui;

    class ConnectionWindow  *connectionWindow;
    class UpdateWindow      *updateWindow;
    class ConsoleWindow     *consoleWindow;
    class GLWindow          *glWindow;

    QLabel statisticsLabel;
    QLabel connectionLabel;
    QTimer timer;

    QTime old_stats_t;
    Connection::Statistics old_stats;

    void addDockWindow(Qt::DockWidgetArea area, QMainWindow *window);
    void timer_timeout();
    void actionFullscreen_triggered();

    virtual void closeEvent(QCloseEvent *event) override;
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
};

#endif // MAINWINDOW_H