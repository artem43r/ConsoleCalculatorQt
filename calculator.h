#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QObject>
#include <QString>
#include <functional>

/**
 * @brief Асинхронный калькулятор.
 *
 * Арифметические слоты не считают сразу, а запускают вычисление в фоновом
 * потоке через QtConcurrent::run() и тут же возвращают управление.
 * За готовностью QFuture следит QFutureWatcher; когда вычисление закончено,
 * калькулятор излучает resultReady() уже в главном потоке.
 *
 * Чтобы асинхронность была заметна, каждая операция искусственно
 * "тяжёлая": фоновый поток спит delayMs() миллисекунд перед расчётом.
 *
 * Ошибки (деление на ноль, неверный ввод) сообщаются сигналом errorOccurred()
 * сразу, без запуска фоновой задачи.
 */
class Calculator : public QObject
{
    Q_OBJECT

public:
    /// Задержка по умолчанию, имитирующая долгое вычисление.
    static constexpr int DefaultDelayMs = 2000;

    explicit Calculator(QObject *parent = nullptr);

    /// Последний полученный результат.
    double result() const { return m_result; }

    /// Была ли последняя операция ошибочной.
    bool hasError() const { return m_hasError; }

    /// Текст последней ошибки (пустой, если ошибки нет).
    QString errorMessage() const { return m_errorMessage; }

    /// Сколько операций сейчас выполняется в фоне.
    int pendingCount() const { return m_pendingCount; }

    /// Искусственная задержка каждой операции, мс.
    int delayMs() const { return m_delayMs; }

public slots:
    /// Запускает в фоне вычисление a + b.
    void add(double a, double b);

    /// Запускает в фоне вычисление a - b.
    void subtract(double a, double b);

    /// Запускает в фоне вычисление a * b.
    void multiply(double a, double b);

    /// Запускает в фоне вычисление a / b. При b == 0 сразу излучает errorOccurred().
    void divide(double a, double b);

    /// Сбрасывает результат в 0 (синхронно) и излучает resultReady().
    void reset();

    /// Сообщает об ошибке через сигнал errorOccurred().
    void reportError(const QString& message);

    /// Задаёт искусственную задержку операций. Отрицательное значение - ошибка.
    void setDelayMs(int ms);

signals:
    /**
     * @brief Фоновая операция запущена.
     * @param expression запись операции, например "5 + 3"
     */
    void operationStarted(const QString& expression);

    /**
     * @brief Результат готов. Всегда излучается в главном потоке.
     * @param expression запись операции, например "5 + 3" (для reset - "reset")
     * @param result результат вычисления
     */
    void resultReady(const QString& expression, double result);

    /// Произошла ошибка.
    void errorOccurred(const QString &message);

    /// Завершилась последняя из выполнявшихся фоновых операций.
    void allFinished();

private:
    double m_result;
    bool m_hasError;
    QString m_errorMessage;
    int m_pendingCount;
    int m_delayMs;

    /**
     * @brief Запускает операцию в фоновом потоке.
     * @param expression запись операции для сигналов
     * @param compute функция расчёта; выполняется в другом потоке, поэтому
     *        не должна обращаться к полям калькулятора - только к своим копиям данных
     */
    void startOperation(const QString& expression, std::function<double()> compute);

    void setResult(const QString& expression, double value);
};
#endif // CALCULATOR_H
