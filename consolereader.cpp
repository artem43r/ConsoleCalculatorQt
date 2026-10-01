#include "consolereader.h"
#include <QTextStream>

ConsoleReader::ConsoleReader(const QStringList& stopCommands, QObject* parent)
    : QThread(parent)
    , m_stopCommands(stopCommands)
{
}

void ConsoleReader::run()
{
    QTextStream in(stdin);
    QString line;

    while (in.readLineInto(&line)) {
        emit lineRead(line);

        // Команду выхода отправили - дальше читать не нужно,
        // поток завершается сам и не держит программу.
        const QString command = line.trimmed().section(' ', 0, 0);
        if (m_stopCommands.contains(command, Qt::CaseInsensitive))
            return;
    }

    emit inputClosed();
}
