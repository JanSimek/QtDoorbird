#ifndef DOORBIRDCLIENT_H
#define DOORBIRDCLIENT_H

#include <QObject>
#include <QtNetwork/QNetworkAccessManager>

class QUdpSocket;
class QUrl;

class DoorbirdClient : public QObject
{
    Q_OBJECT
public:
     explicit DoorbirdClient(QUrl url, QObject *parent = nullptr);
    ~DoorbirdClient();


public slots:
    void unlock();
    void slotAuthenticationRequired(QNetworkReply *, QAuthenticator *authenticator);

signals:
    void doorbell();

private:

    void requestToken();
    void sendAudioMessage();
    void transmitAudio();
    void requestUnlock();
    void receiveBroadcast();

    struct DoorbirdPkt {
        unsigned char id[3];
        unsigned char version;
        uint32_t opslimit;
        uint32_t memlimit;
        unsigned char salt[16];
        unsigned char nonce[8];
        unsigned char ciphertext[34];
    };

    struct DoorbirdEvent {
        char intercom_id[6];
        char event [8];
        unsigned timestamp;
    };

    DoorbirdEvent decrypt_broadcast_notification(const DoorbirdPkt *notification, const unsigned char* password);
    unsigned char* stretchPasswordArgon(const char *password, unsigned char *salt, unsigned* oplimit, unsigned* memlimit);

    QTcpSocket *m_tcpSocket;
    QUdpSocket *m_udpSocket;

    QNetworkAccessManager *m_networkManager;
    QUrl m_url;
    QString m_securityToken;

    const QString API_TOKEN = "/bha-api/getsession.cgi";
    const QString API_UNLOCK = "/bha-api/open-door.cgi";
    const QString API_AUDIO_TRANSMIT = "/bha-api/audio-transmit.cgi";
};

#endif // DOORBIRDCLIENT_H
