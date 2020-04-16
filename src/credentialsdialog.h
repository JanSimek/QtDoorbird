#ifndef CREDENTIALSDIALOG_H
#define CREDENTIALSDIALOG_H

#include <QDialog>

namespace Ui {
class CredentialsDialog;
}

class CredentialsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CredentialsDialog(QWidget *parent = nullptr);
    ~CredentialsDialog();

    QString username() const;
    void setUsername(const QString &username);

    QString password() const;
    void setPassword(const QString &password);

private slots:
    void on_confirmButton_clicked();

private:
    Ui::CredentialsDialog *ui;

    QString m_username;
    QString m_password;
};

#endif // CREDENTIALSDIALOG_H
