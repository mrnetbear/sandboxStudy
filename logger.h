#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QTextEdit>
#include <QDateTime>
#include <QMutex>
#include <QFile>

class Logger : public QObject
{
    Q_OBJECT

public:
    static Logger* instance();

    void setTextEdit(QTextEdit* textEdit);
    void log(const QString& message, const QString& level = "INFO");
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);

    void clear();
    void saveToFile(const QString& fileName);
    QString getLogText() const;

signals:
    void newLogMessage(const QString& formattedMessage);

private:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    static Logger* m_instance;
    QTextEdit* m_textEdit;
    QStringList m_buffer;
    QMutex m_mutex;
    QFile m_logFile;

    QString formatMessage(const QString& message, const QString& level);
};

#endif // LOGGER_H
