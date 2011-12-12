#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>
#include <QtCore>

#include "../QSCPIDev/qscpidev.h"

QSCPIDev::Sense_t QSCPIDev::SenseVolt = "CONF:VOLT";
QSCPIDev::Sense_t QSCPIDev::SenseRes = "CONF:RES";
QSCPIDev::Sense_t QSCPIDev::SenseTemp = "CONF:TEMP TC,K,";

QSCPIDev::QSCPIDev() :
    QSerial(), errorno(0), errorstr("")
{
}

QSCPIDev::~QSCPIDev()
{
    close();
}

void QSCPIDev::close()
{
    QSerial::close();
}

bool QSCPIDev::current(double *i)
{
    QString resp;
    if (!sendQuery(&resp, "SOUR:CURR?"))
        return false;

    bool ok;
    *i = QVariant(resp).toDouble(&ok);

    return ok;
}

int QSCPIDev::error() const
{
    return errorno;
}

QString QSCPIDev::errorString() const
{
    if (errorno == ERR_QSERIAL)
        return QSerial::errorString();
    return errorstr;
}

QString QSCPIDev::formatCmd(const QString &cmd, const Channels_t &channels)
{
    if (!channels.size()) {
        return cmd;
    }

    QStringList ch;
    foreach(int channel, channels) {
        ch.append(QVariant(channel).toString());
    }

    QString format("%1 (@%2)");

    return format.arg(cmd).arg(ch.join(","));
}

bool QSCPIDev::init()
{
    return sendCmd("INIT", 2000000);
}

bool QSCPIDev::open(const QString &port, BaudeRate_t baudeRate)
{
    const long timeout_usec = (10l * 1000000l) / 9600l;

    close();

    if (!QSerial::open(port, baudeRate, 300000, timeout_usec)) {
        errorno = ERR_QSERIAL;
        return false;
    }

    if (!write("\n")) {
        errorno = ERR_QSERIAL;
        QSerial::close();
        return false;
    }
    if (!sendCmd("*RST;*CLS", 500000))
        return false;
    if (!sendCmd("SYST:REM"))
        return false;
    routeChannelsClosed.clear();

    return true;
}

bool QSCPIDev::output(bool *enabled)
{
    QString resp;

    if (!sendQuery(&resp, "OUTP?"))
        return false;

    if (resp == "1") {
        *enabled = true;
        return true;
    }
    if (resp == "0") {
        *enabled = false;
        return true;
    }

    errorno = ERR_RESULT;
    errorstr = "ScpiDev::output invalid result";
    return false;
}

bool QSCPIDev::read(QStringList *values, long timeout_usec)
{
    QString s;

    if (!sendQuery(&s, "READ?", timeout_usec))
        return false;

    *values = s.split(",", QString::SkipEmptyParts);

    for (QStringList::iterator idata(values->begin()); idata != values->end(); ++idata) {
        *idata = idata->trimmed();
    }

    return true;
}

bool QSCPIDev::recvResponse(QString *resp, long timeout_usec)
{
    errorno = ERR_OK;
    errorstr = "";

    if (!readLine(resp, 1024, timeout_usec)) {
        errorno = ERR_QSERIAL;
        return false;
    }
    *resp = resp->trimmed();
    if (*resp == "1") {
        resp->clear();
        return true;
    }
    if (resp->endsWith(";1")) {
        resp->resize(resp->size() - 2);
        return true;
    }

    errorno = ERR_RESULT;
    errorstr = "ScpiDev::recvResponse invalid result";
    return false;
}

bool QSCPIDev::sendCmd(const QString &cmd, long timeout_usec)
{
    QString resp;

    if (!sendQuery(&resp, cmd, timeout_usec))
        return false;

    if (!resp.isEmpty()) {
        errorno = ERR_RESULT;
        errorstr = "ScpiDev::sendCmd nonempty response for nonquery";

        return false;
    }

    return true;
}

bool QSCPIDev::sendCmd(const QString &cmd, const Channels_t &channels, long timeout_usec)
{
    QString resp;

    if (!sendQuery(&resp, cmd, channels, timeout_usec))
        return false;

    if (!resp.isEmpty()) {
        errorno = ERR_RESULT;
        errorstr = "ScpiDev::sendCmd nonempty response";

        return false;
    }

    return true;
}

bool QSCPIDev::sendQuery(QString *resp, const QString &cmd, long timeout_usec)
{
    errorno = 0;
    errorstr = "";

    QString _cmd(cmd + ";*OPC?\n");
    if (!write(_cmd)) {
        errorno = ERR_QSERIAL;
        return false;
    }

    if (!recvResponse(resp, timeout_usec))
        return false;

    return true;
}

bool QSCPIDev::sendQuery(QString *resp, const QString &cmd, const Channels_t &channels, long timeout_usec)
{
    QString _cmd(formatCmd(cmd, channels));

    return sendQuery(resp, _cmd, timeout_usec);
}

bool QSCPIDev::setCurrent(double current)
{
    QString cmd("SOUR:CURR %1");

    return sendCmd(cmd.arg(current));
}

bool QSCPIDev::setOutput(bool enabled)
{
    if (enabled)
        return sendCmd("OUTP 1");
    else
        return sendCmd("OUTP 0");
}

bool QSCPIDev::setRoute(Channels_t closeChannels)
{
    Channels_t openChannels(routeChannelsClosed);
    Channels_t closedChannels(closeChannels);

    foreach (Channel_t ch, closeChannels) {
        openChannels.removeOne(ch);
    }

    foreach (Channel_t ch, routeChannelsClosed) {
        closeChannels.removeOne(ch);
    }

    if(!openChannels.isEmpty()) {
        if (!sendCmd("ROUT:OPEN", openChannels))
            return false;
    }
    routeChannelsClosed.clear();

    if (!closeChannels.isEmpty()) {
        if (!sendCmd("ROUT:CLOS", closeChannels))
            return false;
    }

    routeChannelsClosed = closedChannels;

    return true;
}

bool QSCPIDev::setScan(Channel_t channel)
{
    QString cmd("ROUT:SCAN (@%1);:INIT");

    return sendCmd(cmd.arg(channel), 2000000);
}

bool QSCPIDev::setScan(Channels_t channels)
{
    QString cmd("ROUT:SCAN");

    cmd = formatCmd(cmd, channels).append(";:INIT");
    return sendCmd(cmd, 2000000);
}

bool QSCPIDev::setSense(Sense_t sense, Channels_t channels)
{
    return sendCmd(sense, channels);
}
