#include <QCoreApplication>
#include <QTextStream>
#include <QStringList>
#include "calculator.h"
#include "consolereader.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static const QStringList kQuitCommands = {"quit", "exit"};

void printHelp(QTextStream& out)
{
    out << "Доступные команды:\n";
    out << " add <a> <b> - сложение\n";
    out << " sub <a> <b> - вычитание\n";
    out << " mul <a> <b> - умножение\n";
    out << " div <a> <b> - деление\n";
    out << " reset - сброс\n";
    out << " delay <мс> - задержка, имитирующая долгое вычисление\n";
    out << " help - эта справка\n";
    out << " quit - выход\n";
}

// Разбирает введённую строку и вызывает нужный слот.
// Возвращает false, если пользователь попросил выйти.
bool processLine(const QString& rawLine, Calculator& calc, QTextStream& out)
{
    const QString line = rawLine.trimmed();

    if (line.isEmpty())
        return true;

    const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    const QString command = parts.value(0).toLower();

    if (kQuitCommands.contains(command))
        return false;

    if (command == "help") {
        printHelp(out);
        return true;
    }

    if (command == "reset") {
        calc.reset();
        return true;
    }

    if (command == "delay") {
        bool ok;
        const int ms = parts.value(1).toInt(&ok);

        if (parts.size() != 2 || !ok) {
            calc.reportError("Неверный формат. Используйте: delay <мс>");
            return true;
        }

        calc.setDelayMs(ms);
        if (calc.delayMs() == ms)
            out << "Задержка: " << ms << " мс\n";
        return true;
    }

    if (parts.size() != 3) {
        calc.reportError("Неверный формат. Используйте: <команда> <a> <b>");
        return true;
    }

    bool ok1;
    bool ok2;

    const double a = parts[1].toDouble(&ok1);
    const double b = parts[2].toDouble(&ok2);

    if (!ok1 || !ok2) {
        calc.reportError("Не удалось преобразовать операнды в числа");
        return true;
    }

    if (command == "add")
        calc.add(a, b);
    else if (command == "sub")
        calc.subtract(a, b);
    else if (command == "mul")
        calc.multiply(a, b);
    else if (command == "div")
        calc.divide(a, b);
    else
        calc.reportError("Неизвестная команда: " + command);

    return true;
}

int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    QCoreApplication app(argc, argv);

    QTextStream out(stdout);

    Calculator calc;

    // Чтение консоли в отдельном потоке, чтобы главный поток крутил цикл событий
    ConsoleReader reader(kQuitCommands);

    QObject::connect(&calc, &Calculator::operationStarted,
                     [&out](const QString& expression) {
                         out << "Запущено: " << expression << "\n";
                         out.flush();
                     });

    QObject::connect(&calc, &Calculator::resultReady,
                     [&out](const QString& expression, double result) {
                         if (expression == "reset")
                             out << "Результат: " << result << "\n";
                         else
                             out << "\nРезультат: " << expression << " = " << result << "\n> ";
                         out.flush();
                     });

    QObject::connect(&calc, &Calculator::errorOccurred,
                     [&out](const QString& msg) {
                         out << "Ошибка: " << msg << "\n";
                         out.flush();
                     });

    // Перед выходом дожидаемся операций, которые ещё считаются
    auto finish = [&]() {
        if (calc.pendingCount() > 0) {
            out << "Ожидание завершения операций: " << calc.pendingCount() << "\n";
            out.flush();
            QObject::connect(&calc, &Calculator::allFinished, &app, [&]() {
                out << "\nДо свидания!\n";
                out.flush();
                app.quit();
            });
            return;
        }

        out << "До свидания!\n";
        out.flush();
        app.quit();
    };

    // Контекст &app: лямбда выполняется в главном потоке (Qt::QueuedConnection)
    QObject::connect(&reader, &ConsoleReader::lineRead, &app,
                     [&](const QString& line) {
                         if (!processLine(line, calc, out)) {
                             finish();
                             return;
                         }

                         out << "> ";
                         out.flush();
                     });

    QObject::connect(&reader, &ConsoleReader::inputClosed, &app, finish);

    out << "=== Консольный калькулятор на Qt ===\n";

    printHelp(out);

    out << "\n> ";
    out.flush();

    reader.start();

    const int exitCode = app.exec();

    reader.wait();

    return exitCode;
}
