#include "logger.h"
#include "qregularexpression.h"
#include <QScrollBar>
#include <QTextStream>

Logger* Logger::m_instance = nullptr;

Logger* Logger::instance()
{
    if (!m_instance) {
        m_instance = new Logger();
    }
    return m_instance;
}

Logger::Logger(QObject *parent)
    : QObject(parent)
    , m_textEdit(nullptr)
{
    // Опционально: открываем файл для записи лога
    QString logFileName = QDateTime::currentDateTime().toString("yyyyMMdd") + "_log.txt";
    m_logFile.setFileName(logFileName);
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::setTextEdit(QTextEdit* textEdit)
{
    m_textEdit = textEdit;
    if (m_textEdit) {
        m_textEdit->setReadOnly(true);
        m_textEdit->setFontFamily("Monospace");
        m_textEdit->setFontPointSize(9);
    }
}

QString Logger::formatMessage(const QString& message, const QString& level)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString color;

    if (level == "ERROR") color = "red";
    else if (level == "WARNING") color = "orange";
    else if (level == "DEBUG") color = "gray";
    else color = "black";

    QString formatted = QString("[%1] <font color='%2'>[%3]</font> %4")
                            .arg(timestamp)
                            .arg(color)
                            .arg(level)
                            .arg(message);

    return formatted;
}

void Logger::log(const QString& message, const QString& level)
{
    QMutexLocker locker(&m_mutex);

    QString formatted = formatMessage(message, level);

    // Сохраняем в буфер
    m_buffer.append(formatted);
    while (m_buffer.size() > 10000) {
        m_buffer.removeFirst();
    }

    // Выводим в QTextEdit
    if (m_textEdit) {
        m_textEdit->append(formatted);

        // Автопрокрутка
        QScrollBar* scrollBar = m_textEdit->verticalScrollBar();
        if (scrollBar && scrollBar->value() == scrollBar->maximum()) {
            scrollBar->setValue(scrollBar->maximum());
        }
    }

    // Сохраняем в файл
    if (m_logFile.isOpen()) {
        QTextStream stream(&m_logFile);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
               << " [" << level << "] " << message << "\n";
        stream.flush();
    }

    // Отправляем сигнал
    emit newLogMessage(formatted);

    // Выводим в консоль
    fprintf(stderr, "%s\n", qPrintable(formatted));
    fflush(stderr);
}

void Logger::debug(const QString& message)
{
    log(message, "DEBUG");
}

void Logger::info(const QString& message)
{
    log(message, "INFO");
}

void Logger::warning(const QString& message)
{
    log(message, "WARNING");
}

void Logger::error(const QString& message)
{
    log(message, "ERROR");
}

void Logger::clear()
{
    QMutexLocker locker(&m_mutex);
    m_buffer.clear();
    if (m_textEdit) {
        m_textEdit->clear();
    }
}

void Logger::saveToFile(const QString& fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        for (const QString& line : m_buffer) {
            // Удаляем HTML теги
            QString plainText = line;
            plainText.remove(QRegularExpression("<[^>]*>"));
            stream << plainText << "\n";
        }
        file.close();
        info(QString("Log saved to: %1").arg(fileName));
    } else {
        error(QString("Failed to save log to: %1").arg(fileName));
    }
}

QString Logger::getLogText() const
{
    QString result;
    for (const QString& line : m_buffer) {
        QString plainText = line;
        plainText.remove(QRegularExpression("<[^>]*>"));
        result += plainText + "\n";
    }
    return result;
}
