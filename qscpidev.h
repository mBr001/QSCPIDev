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

    /** SCPI version informations. */
    struct Version {
        int year;
        int approved;

        Version() :
            year(-1), approved(-1)
        { }

        bool isValid() const
        { return year > 0 && approved >= 0; }
    };

    static Sense_t SenseVolt;
    static Sense_t SenseRes;
    static Sense_t SenseTemp;

    QSCPIDev();
    ~QSCPIDev();
    void close();
    bool current(double *i);
    bool idn(QString *resp);
    int error() const;
    QString errorString() const;
    bool init();
    bool open(const QString &port, BaudeRate_t baudeRate = QSerial::BAUDE9600,
              bool setRemote = true);
    bool output(bool *enabled);
    bool read(QStringList *values, long timeout_usec = 0);
    bool setCurrent(double current);
    bool setOutput(bool enabled);
    bool setRoute(Channels_t closeChannels, Channel_t min, Channel_t max);
    bool setScan(Channel_t channel);
    bool setScan(const Channels_t &channels);
    bool setSense(Sense_t sense, const Channels_t &channels,
                  const QStringList &params = QStringList());
    bool systemRemote();
    Version systemVersion();

protected:
    QString formatCmd(const QString &cmd, const QStringList &params, const Channels_t &channels);
    bool recvResponse(QString *resp, long timeout_usec = 0);
    bool sendCmd(const QString &cmd, long timeout_usec = 0);
    bool sendCmd(const QString &cmd, const Channels_t &channels, long timeout_usec = 0);
    bool sendCmd(const QString &cmd, const QStringList &params,
                 const Channels_t &channels, long timeout_usec = 0);
    bool sendQuery(QString *resp, const QString &cmd, long timeout_usec = 0);

private:
    int errorno;
    const char *errorstr;
};

#endif // QSCPIDEV_H
