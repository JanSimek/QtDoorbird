#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "doorbirdclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QSystemTrayIcon;
class QMenu;
class QMediaPlayer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void openSettings();

protected:
    void keyReleaseEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void startVideo(QUrl url);


private:
    Ui::MainWindow *ui;

    QMediaPlayer *m_player;
    DoorbirdClient *m_doorbird;

    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayIconMenu;
};
#endif // MAINWINDOW_H
