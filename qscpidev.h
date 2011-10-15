#ifndef QSCPIDEV_H
#define QSCPIDEV_H

#include "qserial.h"
#include <qlist.h>
#include <qstringlist.h>

class QSCPIDev : protected QSerial
{
public:
    typedef const char *Sense_t;
    typedef int Channel_t;
    typedef QList<Channel_t> Channels_t;
    typedef enum {
        ERR_OK = 0,
        ERR_QSERIAL = 1,
        ERR_RESULT = 2
    } error_t;

    Channels_t routeChannelsClosed;

    static Sense_t SenseVolt;
    static Sense_t SenseRes;

    QSCPIDev();
    ~QSCPIDev();
    void close();
    bool current(double *i);
    int error() const;
    QString errorStr() const;
    bool init();
    bool open(const QString &port, BaudeRate_t baudeRate = QSerial::Baude9600);
    bool output(bool *enabled);
    bool read(QStringList *values);
    bool setCurrent(double current);
    bool setOutput(bool enabled);
    bool setRoute(Channels_t closeChannels);
    bool setScan(Channel_t channel);
    bool setScan(Channels_t channels);
    bool setSense(Sense_t sense, Channels_t channels);

protected:
    QString formatCmd(const QString &cmd, const Channels_t &channels);
    bool recvResponse(QString *resp, long timeout = 0);
    bool sendCmd(const QString &cmd, long timeout = 0);
    bool sendCmd(const QString &cmd, const Channels_t &channels, long timeout = 0);
    bool sendQuery(QString *resp, const QString &cmd, long timeout = 0);
    bool sendQuery(QString *resp, const QString &cmd, const Channels_t &channels, long timeout = 0);

private:
    int errorno;
    const char *errorstr;
};

#endif // QSCPIDEV_H
