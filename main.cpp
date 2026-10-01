#include <QCoreApplication>
#include <QTextStream>
#include <QStringList>
#include "calculator.h"
#include "consolereader.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// Команды, по которым программа завершается
static const QStringList kQuitCommands = {"quit", "exit"};

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

// Разбирает одну введённую строку и вызывает нужный слот калькулятора.
// Возвращает false, если пользователь попросил выйти.
bool processLine(const QString& rawLine, Calculator& calc, QTextStream& out)
{
    const QString line = rawLine.trimmed();

    if (line.isEmpty())
        return true;

    const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    const QString command = parts.value(0).toLower();

    // Выход
    if (kQuitCommands.contains(command))
        return false;

    // Справка
    if (command == "help") {
        printHelp(out);
        return true;
    }

    // Сброс
    if (command == "reset") {
        calc.reset();
        return true;
    }

    // Для арифметики должно быть:
    // команда + два числа
    if (parts.size() != 3) {
        calc.reportError("Неверный формат. Используйте: <команда> <a> <b>");
        return true;
    }

    bool ok1;
    bool ok2;

    const double a = parts[1].toDouble(&ok1);
    const double b = parts[2].toDouble(&ok2);

    // Если хотя бы одно значение не число
    if (!ok1 || !ok2) {
        calc.reportError("Не удалось преобразовать операнды в числа");
        return true;
    }

    // Вызываем соответствующий слот
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

    // Чтение клавиатуры живёт в своём потоке,
    // чтобы главный поток был свободен для цикла событий.
    ConsoleReader reader(kQuitCommands);

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

    // Строка пришла из потока чтения. Третий аргумент &app - "контекст":
    // лямбда выполнится в потоке app (главном), поэтому Qt доставит сигнал
    // через очередь событий (Qt::QueuedConnection), а не вызовет её в чужом потоке.
    QObject::connect(&reader, &ConsoleReader::lineRead, &app,
                     [&](const QString& line) {
                         if (!processLine(line, calc, out)) {
                             out << "До свидания!\n";
                             out.flush();
                             app.quit();
                             return;
                         }

                         out << "> ";
                         out.flush();
                     });

    // Ввод закончился (Ctrl+Z) - выходим
    QObject::connect(&reader, &ConsoleReader::inputClosed,
                     &app, &QCoreApplication::quit);

    out << "=== Консольный калькулятор на Qt ===\n";

    printHelp(out);

    out << "\n> ";
    out.flush();

    reader.start();

    // Цикл событий: главный поток ждёт и обрабатывает сигналы
    // от потока ввода (и, дальше, от фоновых вычислений).
    const int exitCode = app.exec();

    // Поток чтения к этому моменту уже завершился сам
    // (после quit или конца ввода) - дожидаемся его.
    reader.wait();

    return exitCode;
}
