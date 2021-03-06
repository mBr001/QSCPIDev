#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <unistd.h>
#include <QtCore>

#include "qserial.h"

QSerial::QSerial() :
    errorno(0), errorstr(""), fd(-1)
{
}

QSerial::~QSerial()
{
    close();
}

void QSerial::close()
{
    errorno = 0;
    errorstr = "";

    ::close(fd);
    fd = -1;
}

int QSerial::error() const
{
    return errorno;
}

QString QSerial::errorString() const
{
    return QString(errorstr) + strerror(errorno);
}

bool QSerial::isOpen() const
{
    return (fd != -1);
}

bool QSerial::isLine(const char *buf, ssize_t size) const
{
    // empty line is not real line
    if (size <= 1)
        return false;

    if (buf[size - 1] == '\n' || buf[size - 1] == '\r')
        return true;

    return false;
}

QStringList QSerial::list()
{
    QStringList ports;

#ifdef __linux__
    QString root("/dev/");
    QDir dir(root);
    QStringList filters;
    filters << "ttyS*" << "ttyUSB*";

    dir.setNameFilters(filters);
    ports = dir.entryList(QDir::System, QDir::Name | QDir::LocaleAware);
    ports.replaceInStrings(QRegExp("^"), root);
#else
#error "Not implemented!"
#endif
    return ports;
}

bool QSerial::open(const char *port, BaudeRate_t bauderate, long timeout_usec,
                   long timeoutPerChar)
{
    errorno = 0;
    errorstr = "";

    fd = ::open(port, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        errorno = errno;
        errorstr = "QSerial::open open ";
        return false;
    }

    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = CS8 | CREAD | CLOCAL;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 5;
    cfsetispeed(&tio, bauderate);
    cfsetospeed(&tio, bauderate);

    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        errorno = errno;
        errorstr = "QSerial::open tcsetattr ";
        ::close(fd);

        return false;
    }

    this->timeoutOffs = timeout_usec;
    this->timeoutPerChar = timeoutPerChar;
    return fd;
}

bool QSerial::open(const QString &port, BaudeRate_t bauderate, long timeout,
                   long timeoutPerChar)
{
    return open(qPrintable(port), bauderate, timeout, timeoutPerChar);
}

bool QSerial::readLine(QString *str, ssize_t maxSize, long timeout = 0)
{
    char buf[maxSize + 1];
    ssize_t len;

    errorno = 0;
    errorstr = "";

    len = readLine(buf, maxSize, timeout);
    if (len == -1) {
        errorno = errno;
        errorstr = "QSerial::readLine readLine ";

        return false;
    }
    buf[len] = 0;
    *str = buf;

    return true;
}

ssize_t QSerial::readLine(char *buf, ssize_t count, long timeout_usec)
{
    const char *buf_ = buf;
    fd_set readfds;
    int ret;
    ssize_t size = 0;
    struct timeval timeout_s;

    timeout_usec = timeout_usec ? timeout_usec : timeoutOffs;
    timeout_s.tv_sec = timeout_usec / 1000000l;
    timeout_s.tv_usec = timeout_usec % 1000000l;

    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);

    do {
        ssize_t size_;

        ret = select(fd + 1, &readfds, NULL, NULL, &timeout_s);
        if (ret == -1) {
                return -1;
        }
        size_ = ::read(fd, buf, count);
        if (size_ == -1) {
            return size;
        }
        timeout_s.tv_usec += timeoutPerChar * size_;
        timeout_s.tv_sec += timeout_s.tv_usec / 1000000l;
        timeout_s.tv_usec %= 1000000l;
        size += size_;
        count -= size_;
        buf += size_;
        if (isLine(buf_, size)) {
            return size;
        }
    } while (count > 0);

    errno = ERANGE;
    return -1;
}

bool QSerial::write(const char *str)
{
    errorno = 0;
    errorstr = "";

    ssize_t len(strlen(str));
    if (::write(fd, str, len) != len) {
        errorno = errno;
        errorstr = "QSerial::write write ";

        return false;
    }
    return true;
}

bool QSerial::write(const QString &str)
{
    errorno = 0;
    errorstr = "";

    QByteArray bytes(str.toUtf8());
    if (::write(fd, bytes.constData(), bytes.length()) != bytes.length()) {
        errorno = errno;
        errorstr = "QSerial::write write ";

        return false;
    }
    return true;
}
