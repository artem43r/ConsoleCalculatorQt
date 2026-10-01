#include <QCoreApplication>
#include <QTextStream>
#include <QStringList>
#include "calculator.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// Выводит список доступных команд
void printHelp(QTextStream& out)
{
    out << "Доступные команды:\n";
    out << " add <a> <b> - сложение\n";
    out << " sub <a> <b> - вычитание\n";
    out << " mul <a> <b> - умножение\n";
    out << " div <a> <b> - деление\n";
    out << " reset - сброс\n";
    out << " help - эта справка\n";
    out << " quit - выход\n";
}

int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    QCoreApplication app(argc, argv);

    QTextStream in(stdin);
    QTextStream out(stdout);

    Calculator calc;

    // Когда калькулятор успешно посчитал результат,
    // выводим его в консоль.
    QObject::connect(&calc, &Calculator::resultReady,
                     [&out](double result) {
                         out << "Результат: " << result << "\n";
                         out.flush();
                     });

    // Когда произошла ошибка,
    // выводим сообщение об ошибке.
    QObject::connect(&calc, &Calculator::errorOccurred,
                     [&out](const QString& msg) {
                         out << "Ошибка: " << msg << "\n";
                         out.flush();
                     });

    out << "=== Консольный калькулятор на Qt ===\n";

    printHelp(out);

    out << "\n> ";
    out.flush();

    QString line;

    while (in.readLineInto(&line)) {
        line = line.trimmed();

        if (line.isEmpty()) {
            out << "> ";
            out.flush();
            continue;
        }

        QStringList parts =
            line.split(' ', Qt::SkipEmptyParts);

        QString command =
            parts.value(0).toLower();

        // Выход
        if (command == "quit" || command == "exit") {
            out << "До свидания!\n";
            break;
        }

        // Справка
        if (command == "help") {
            printHelp(out);

            out << "> ";
            out.flush();
            continue;
        }

        // Сброс
        if (command == "reset") {
            calc.reset();

            out << "> ";
            out.flush();
            continue;
        }

        // Для арифметики должно быть:
        // команда + два числа
        if (parts.size() != 3) {
            calc.reportError(
                "Неверный формат. Используйте: <команда> <a> <b>"
                );

            out << "> ";
            out.flush();
            continue;
        }

        bool ok1;
        bool ok2;

        double a = parts[1].toDouble(&ok1);
        double b = parts[2].toDouble(&ok2);

        // Если хотя бы одно значение не число
        if (!ok1 || !ok2) {
            calc.reportError(
                "Не удалось преобразовать операнды в числа"
                );

            out << "> ";
            out.flush();
            continue;
        }

        // Вызываем соответствующий слот
        if (command == "add") {
            calc.add(a, b);
        }
        else if (command == "sub") {
            calc.subtract(a, b);
        }
        else if (command == "mul") {
            calc.multiply(a, b);
        }
        else if (command == "div") {
            calc.divide(a, b);
        }
        else {
            calc.reportError(
                "Неизвестная команда: " + command
                );
        }

        out << "> ";
        out.flush();
    }

    return 0;
}
