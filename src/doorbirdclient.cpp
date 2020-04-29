#include "doorbirdclient.h"
#include "credentialsdialog.h"

#include <QDialog>
#include <QFile>
#include <QUrlQuery>
#include <QtNetwork>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QJsonDocument>
#include <QJsonObject>
#include <QEventLoop>
#include <QThread>
#include <QUrl>

#include <sodium.h>
#include <arpa/inet.h>


/*
 *
 * . You can split
the package in two sections. The first one contains information about the package,
the second part about encryption with payload. Please notice, we are also sending
keep alive broadcasts every 7 seconds on this two ports, these packets are not
relevant for the decryption of event broadcasts, you can skip them.
 */


#define CRYPTO_SALT_BYTES 16
#define CRYPTO_ARGON_OUT_SIZE 32

DoorbirdClient::DoorbirdClient(QUrl url, QObject *parent) : QObject(parent), m_url(url)
{
    m_udpSocket = new QUdpSocket(this);
    // After an event occurred, the DoorBird sends multiple identical UDP-Broadcasts on
    // the ports 6524 and 35344 for every user and every connected device
    m_udpSocket->bind(6524, QUdpSocket::ShareAddress);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &DoorbirdClient::receiveBroadcast);

    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::authenticationRequired, this, &DoorbirdClient::slotAuthenticationRequired);
}

DoorbirdClient::~DoorbirdClient()
{
    m_networkManager->deleteLater();
}

void DoorbirdClient::unlock()
{
    requestToken();
}

void DoorbirdClient::sendAudioMessage()
{
    m_tcpSocket = new QTcpSocket(this);

    connect(m_tcpSocket, &QTcpSocket::connected, this, &DoorbirdClient::transmitAudio);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, [=]() {
        qDebug() << "disconnected...";
    });
    connect(m_tcpSocket, &QTcpSocket::bytesWritten, this, [=](qint64 bytes) {
        qDebug() << bytes << " bytes written... " ;
    });
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, [=]() {
        qDebug() << "reading...: " << m_tcpSocket->readAll();
    });

    qDebug() << "connecting... " << m_url.host();

    // this is not blocking call
    m_tcpSocket->connectToHost(m_url.host(), 80);

    if(!m_tcpSocket->waitForConnected(5000))
    {
        qDebug() << "Error: " << m_tcpSocket->errorString();
    }

    m_tcpSocket->disconnect();
    m_tcpSocket->disconnectFromHost();
    m_tcpSocket->deleteLater();
}

void DoorbirdClient::requestToken()
{
    m_url.setPath(API_TOKEN);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(m_url));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray result = reply->readAll();

            QJsonDocument doc = QJsonDocument::fromJson(result);
            m_securityToken = doc["BHA"]["SESSIONID"].toString();

            qDebug() << "Token: " << m_securityToken;
        } else {
            qDebug() << "Error: " + QString(reply->errorString());
        }

        QVariant statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
        qDebug() << "Status code: " + statusCode.toString();

        sendAudioMessage();
    });
    loop.exec();
}

void DoorbirdClient::requestUnlock()
{
    m_url.setPath(API_UNLOCK);
    QUrlQuery query;
    query.addQueryItem("sessionid", m_securityToken);
    m_url.setQuery(query);
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(m_url));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Reply: " << reply->readAll();

        } else {
            qDebug() << "Error: " + QString(reply->errorString());
        }

        QVariant statusCode = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
        qDebug() << "Status code: " + statusCode.toString();

        //        sendAudioMessage();
    });


}

void DoorbirdClient::receiveBroadcast()
{
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray packet;
        packet.resize(int(m_udpSocket->pendingDatagramSize()));
        m_udpSocket->readDatagram(packet.data(), packet.size());

        //        qDebug() << "Received message: " << packet;

        const QByteArray ident {"\xDE\xAD\xBE"};

        if (!packet.startsWith(ident)) {
            continue; // not a Doorbird packet
        }

        // FIXME: not cross-platform, alignment and endianness should match
        // https://stackoverflow.com/questions/24185337/receive-data-with-udp-protocol-and-readdatagram
        DoorbirdPkt *pkt = reinterpret_cast<DoorbirdPkt*>(packet.data());

        qDebug() << "Ident: " << pkt->id;
        qDebug() << "Version: " << pkt->version;
        qDebug() << "Salt: " << pkt->salt;
        qDebug() << "Nonce: " << pkt->nonce;

        char pass5[6];
        strncpy(pass5, "password", 5);
        pass5[5] = 0;
        unsigned opslimit = ntohl(pkt->opslimit);
        unsigned memlimit = ntohl(pkt->memlimit);

        unsigned char* stretchPass = stretchPasswordArgon(pass5, pkt->salt, &opslimit, &memlimit);
        if (stretchPass == NULL) {
            qDebug("Error making stretchpass!");
        }

    }

}

void DoorbirdClient::transmitAudio()
{
    qDebug() << "connected...";

    const char* postHeader = QString("POST " + API_AUDIO_TRANSMIT + "?sessionid=" + m_securityToken + " HTTP/1.0\r\n").toUtf8().constData();
    m_tcpSocket->write(postHeader);
    m_tcpSocket->write("Content-Type: audio/basic\r\n");
    m_tcpSocket->write("Content-Length: 9999999\r\n");
    m_tcpSocket->write("Connection: Keep-Alive\r\n");
    m_tcpSocket->write("Cache-Control: no-cache\r\n");
    m_tcpSocket->write("\r\n");

    QFile sound(":/door-unlocked.wav");
    if(!sound.open(QIODevice::ReadOnly)) {
        qDebug() << "Sound file could not be opened";
        m_tcpSocket->disconnectFromHost();
        return;
    }

    qDebug() << "Sending sound bytes";

    // don't ask me why
    QByteArray bytes = sound.readAll();
    int pos = 0, arrsize = bytes.size(), sizeInArray = 2048;
    while(pos < arrsize){
        m_tcpSocket->flush();
        QThread::msleep(200);
        QByteArray arr = bytes.mid(pos, sizeInArray);
        m_tcpSocket->write(arr);
        m_tcpSocket->waitForBytesWritten(1000); // useless?
        pos += arr.size();
    }

    requestUnlock();
}

void DoorbirdClient::slotAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    CredentialsDialog dialog;


    // Did the URL have information? Fill the UI
    // This is only relevant if the URL-supplied credentials were wrong
    //    ui.userEdit->setText(url.userName());
    //    ui.passwordEdit->setText(url.password());

    //    if (dialog.exec() == QDialog::Accepted) {
    //        qDebug() << "User: " << dialog.username() << ", password: " << dialog.password();

    //        authenticator->setUser(dialog.username());
    //        authenticator->setPassword(dialog.password());
    authenticator->setUser("gheetm0001");
    authenticator->setPassword("xnZNWT7n2L");
    //    }
}


unsigned char* DoorbirdClient::stretchPasswordArgon(const char *password, unsigned char *salt, unsigned* oplimit, unsigned* memlimit) {
    if (sodium_is_zero(salt, CRYPTO_SALT_BYTES) ) {
        randombytes_buf(salt, CRYPTO_SALT_BYTES);
        return NULL;
    }
    unsigned char* key = (unsigned char*)malloc(CRYPTO_ARGON_OUT_SIZE);

    if (!*oplimit) {
        *oplimit = crypto_pwhash_argon2i_OPSLIMIT_INTERACTIVE;
    }

    if (!*memlimit) {
        *memlimit = crypto_pwhash_MEMLIMIT_MIN;
    }

    if (crypto_pwhash(key, CRYPTO_ARGON_OUT_SIZE, password, strlen(password), salt, *oplimit, *memlimit, crypto_pwhash_ALG_ARGON2I13)) {
        qDebug("Argon2 Failed");
        *oplimit = 0;
        *memlimit = 0;
        free(key);
        return NULL;
    }
    return key;
}

DoorbirdClient::DoorbirdEvent DoorbirdClient::decrypt_broadcast_notification(const DoorbirdPkt *notification, const unsigned char* password) {
    DoorbirdEvent decrypted = {{0},{0},0};
    int res = crypto_aead_chacha20poly1305_decrypt((unsigned char*)&decrypted, NULL, NULL, notification->ciphertext, sizeof(notification->ciphertext), NULL, 0, notification->nonce, password);
    if(res){
        qDebug("crypto_aead_chacha20poly1305_decrypt() failed %d", res);
        perror(NULL);
    }
    return decrypted;
}



