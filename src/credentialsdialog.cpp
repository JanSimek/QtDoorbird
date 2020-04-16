#include "credentialsdialog.h"
#include "ui_credentialsdialog.h"

CredentialsDialog::CredentialsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CredentialsDialog)
{
    ui->setupUi(this);
}

CredentialsDialog::~CredentialsDialog()
{
    delete ui;
}

void CredentialsDialog::on_confirmButton_clicked()
{
    setUsername(ui->userLineEdit->text());
    setPassword(ui->passwordLineEdit->text());
    accept();
}

QString CredentialsDialog::password() const
{
    return m_password;
}

void CredentialsDialog::setPassword(const QString &password)
{
    m_password = password;
}

QString CredentialsDialog::username() const
{
    return m_username;
}

void CredentialsDialog::setUsername(const QString &username)
{
    m_username = username;
}
