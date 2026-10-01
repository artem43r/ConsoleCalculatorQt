#ifndef CONSOLEREADER_H
#define CONSOLEREADER_H

#include <QThread>
#include <QStringList>

/**
 * @brief Читает строки с клавиатуры в отдельном потоке.
 *
 * Чтение stdin блокирующее: readLine() стоит и ждёт Enter. Если делать это
 * в главном потоке, он не сможет крутить цикл событий (app.exec()) и не получит
 * сигналы от фоновых вычислений. Поэтому чтение вынесено в свой поток,
 * а каждая введённая строка передаётся в главный поток сигналом lineRead().
 */
class ConsoleReader : public QThread
{
    Q_OBJECT

public:
    /**
     * @param stopCommands команды, после которых чтение прекращается (например "quit").
     *        Сама команда всё равно отправляется сигналом lineRead().
     * @param parent родительский объект
     */
    explicit ConsoleReader(const QStringList& stopCommands, QObject* parent = nullptr);

signals:
    /// Пользователь ввёл строку. Излучается из потока чтения.
    void lineRead(const QString& line);

    /// Ввод закончился (Ctrl+Z или конец файла, если stdin перенаправлен).
    void inputClosed();

protected:
    /// Тело потока: цикл чтения строк.
    void run() override;

private:
    QStringList m_stopCommands;
};

#endif // CONSOLEREADER_H
