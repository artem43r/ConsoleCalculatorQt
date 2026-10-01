#ifndef CONSOLEREADER_H
#define CONSOLEREADER_H

#include <QThread>
#include <QStringList>

// Читает строки из консоли в отдельном потоке и передаёт их сигналом lineRead()
class ConsoleReader : public QThread
{
    Q_OBJECT

public:
    // stopCommands - команды, после которых чтение прекращается
    explicit ConsoleReader(const QStringList& stopCommands, QObject* parent = nullptr);

signals:
    void lineRead(const QString& line);

    // Ввод закончился (Ctrl+Z).
    void inputClosed();

protected:
    void run() override;

private:
    QStringList m_stopCommands;
};

#endif // CONSOLEREADER_H
