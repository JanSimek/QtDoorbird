#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QSettings>

#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_Settings_accepted()
{
    QSettings settings;
    settings.setValue("doorbird_ip", ui->editIpAddress->text());
    settings.setValue("rtsp", ui->editRtsp->text());
}
