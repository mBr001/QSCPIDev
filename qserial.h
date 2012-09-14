#ifndef QSERIAL_H
#define QSERIAL_H

#include <QtCore>
#include <termios.h>

class QSerial
{
public:
    typedef enum {
        BAUDE9600 = B9600,
        BAUDE19200 = B19200
    } BaudeRate_t;

    QSerial();
    ~QSerial();
    void close();

    /** @return errno of last error. */
    int error() const;

    /** String describing last error. */
    QString errorString() const;

    /** @return true if port is open, false otherwise. */
    bool isOpen() const;

    /** List all available serial ports. */
    static QStringList list();

    /**
     * Open serial port and set parameters.
     * @param port Serial port name.
     * @param bauderate Bauderate of serial port.
     * @param timeoutOffs Timeout for first read attempt [usec].
     * @param timeoutPerChar Reading time timeout per recieved character [us].
     * @return True on succes, false otherwise.
     *
     * timeoutPerChar increases timeout for each character wich should be readed,
     * this timeout is already set properly by bauderate.
     */
    bool open(const char *port, BaudeRate_t bauderate, long timeoutOffs,
              long timeoutPerChar);

    /**
     * Open serial port and set parameters.
     * @param port Serial port name.
     * @param bauderate bauderate of serial port.
     * @param timeoutOffs Time reserved for reading [usec].
     * @param timeoutPerChar Reading time timeout per recieved character [us].
     * @return True on succes, false otherwise.
     *
     * timeoutPerChar increases timeout for each character wich should be readed,
     * this timeout is already set properly by bauderate.
     */
    bool open(const QString &port, BaudeRate_t bauderate, long timeoutOffs,
              long timeoutPerChar);

    /** Write C string into serial line (not includit trailing zero). */
    bool write(const char *str);

    /** Write UTF-8 string into serial line. */
    bool write(const QString &str);

    /**
     * Read line from serial port.
     * @param str String where to store readed text.
     * @param count Maximal amount of bytes to read.
     * @return True if read succeded, false otherwise.
     */
    bool readLine(QString *str, ssize_t count);
    bool readLine(QString *str, ssize_t maxSize, long timeout_usec);

private:
    int errorno;
    const char *errorstr;
    int fd;
    long timeoutOffs, timeoutPerChar;

    bool isLine(const char *buf, ssize_t size) const;
    ssize_t readLine(char *buf, ssize_t count, long timeout_usec);
};

#endif // QSERIAL_H
