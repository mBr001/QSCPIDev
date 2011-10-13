#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/select.h>
#include <unistd.h>
#include <QtCore>

#include "../QSCPIDev/qscpidev.h"

ScpiDev::Sense_t ScpiDev::SenseVolt = "CONF:VOLT";
ScpiDev::Sense_t ScpiDev::SenseRes = "CONF:RES";

ScpiDev::ScpiDev() :
    QSerial(), errorno(0), errorstr("")
{
}

ScpiDev::~ScpiDev()
{
    close();
}

void ScpiDev::close()
{
    QSerial::close();
}

bool ScpiDev::current(double *i)
{
    QString resp;
    if (!sendQuery(&resp, "SOUR:CURR?"))
        return false;

    bool ok;
    *i = QVariant(resp).toDouble(&ok);

    return ok;
}

int ScpiDev::error() const
{
    return errorno;
}

QString ScpiDev::errorStr() const
{
    if (errorno == ERR_QSERIAL)
        return QSerial::errorStr();
    return errorstr;
}

QString ScpiDev::formatCmd(const QString &cmd, const Channels_t &channels)
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

bool ScpiDev::init()
{
    return sendCmd("INIT", 2000000);
}

bool ScpiDev::open(const QString &port, BaudeRate_t baudeRate)
{
    const long timeout = (10l * 1000000l) / 9600l;

    close();

    if (!QSerial::open(port, baudeRate, 300000, timeout))
        return false;

    if (!write("\n")) {
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

bool ScpiDev::output(bool *enabled)
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

bool ScpiDev::read(QStringList *values)
{
    QString s;

    if (!sendQuery(&s, "READ?"))
        return false;

    *values = s.split(",", QString::SkipEmptyParts);

    for (QStringList::iterator idata(values->begin()); idata != values->end(); ++idata) {
        *idata = idata->trimmed();
    }

    return true;
}

bool ScpiDev::recvResponse(QString *resp, long timeout)
{
    errorno = ERR_OK;
    errorstr = "";

    if (!readLine(resp, 1024, timeout)) {
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

bool ScpiDev::sendCmd(const QString &cmd, long timeout)
{
    QString resp;

    if (!sendQuery(&resp, cmd, timeout))
        return false;

    if (!resp.isEmpty()) {
        errorno = ERR_RESULT;
        errorstr = "ScpiDev::sendCmd nonempty response for nonquery";

        return false;
    }

    return true;
}

bool ScpiDev::sendCmd(const QString &cmd, const Channels_t &channels, long timeout)
{
    QString resp;

    if (!sendQuery(&resp, cmd, channels, timeout))
        return false;

    if (!resp.isEmpty()) {
        errorno = ERR_RESULT;
        errorstr = "ScpiDev::sendCmd nonempty response";

        return false;
    }

    return true;
}

bool ScpiDev::sendQuery(QString *resp, const QString &cmd, long timeout)
{
    errorno = 0;
    errorstr = "";

    QString _cmd(cmd + ";*OPC?\n");
    if (!write(_cmd)) {
        errorno = ERR_QSERIAL;
        return false;
    }

    if (!recvResponse(resp, timeout))
        return false;

    return true;
}

bool ScpiDev::sendQuery(QString *resp, const QString &cmd, const Channels_t &channels, long timeout)
{
    QString _cmd(formatCmd(cmd, channels));

    return sendQuery(resp, _cmd, timeout);
}

bool ScpiDev::setCurrent(double current)
{
    QString cmd("SOUR:CURR %1");

    return sendCmd(cmd.arg(current));
}

bool ScpiDev::setOutput(bool enabled)
{
    if (enabled)
        return sendCmd("OUTP 1");
    else
        return sendCmd("OUTP 0");
}

bool ScpiDev::setRoute(Channels_t closeChannels)
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

bool ScpiDev::setScan(Channel_t channel)
{
    QString cmd("ROUT:SCAN (@%1);:INIT");

    return sendCmd(cmd.arg(channel), 2000000);
}

bool ScpiDev::setScan(Channels_t channels)
{
    QString cmd("ROUT:SCAN");

    cmd = formatCmd(cmd, channels).append(";:INIT");
    return sendCmd(cmd, 2000000);
}

bool ScpiDev::setSense(Sense_t sense, Channels_t channels)
{
    return sendCmd(sense, channels);
}
