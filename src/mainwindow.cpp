#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDebug>

#include <QThread>
#include <QKeyEvent>
#include <QMediaPlayer>

#include <QSettings>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTimer>

#include "settingsdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->statusbar->hide();
    ui->menubar->hide();

    m_trayIcon = new QSystemTrayIcon(QIcon::fromTheme("bell"), this);
    m_trayIconMenu = new QMenu(this);
    m_trayIconMenu->addAction("Exit", [=]() {
        QApplication::quit();
    });
    m_trayIcon->setContextMenu(m_trayIconMenu);
    m_trayIcon->show();

    m_player = new QMediaPlayer(this);

    connect(m_trayIcon, &QSystemTrayIcon::activated, [=](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::ActivationReason::Trigger) {
            show();
            raise();
            activateWindow();
        }
    });

    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
        [=](QMediaPlayer::Error error) {

        // FIXME: bad url?
        // e.g. when the computer is suspended and we lose the connection
        if (error != QMediaPlayer::NoError) {
            m_player->stop();
            m_player->play();
        }
    });

    QSettings settings;

    QString rtspUrl = settings.value("rtsp").toString();
    QString doorbirdUrl = settings.value("doorbird_ip").toString();

    startVideo(QUrl(rtspUrl));

    m_doorbird = new DoorbirdClient(QUrl(doorbirdUrl), this);

    connect(ui->unlockButton, &QPushButton::released, m_doorbird, &DoorbirdClient::unlock);
}

void MainWindow::startVideo(QUrl url) {
    m_player->setVideoOutput(ui->video);
    m_player->setMedia(url);
    m_player->play();
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    QMainWindow::keyReleaseEvent(event);

    switch(event->key()) {
    case Qt::Key_Alt:
        if (ui->menubar->isVisible()) {
            ui->menubar->hide();
        } else {
            ui->menubar->show();
        }
        break;
    case Qt::Key_Space:
        m_doorbird->unlock();
        break;
    /*
    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_player->stop();
        m_player->play();
        break;
    */
    default:
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_trayIcon->isVisible()) {
        hide();
        // TODO: stop video stream?
        event->ignore();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openSettings()
{
    SettingsDialog *settingsDlg = new SettingsDialog();
    connect(settingsDlg, &SettingsDialog::on_Settings_accepted, this, [=]() {
        // TODO
    });
    settingsDlg->setModal(true);
    settingsDlg->show();
}

