#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <QObject>
#include <QString>
#include <functional>

/**
 * @brief Асинхронный калькулятор: операции выполняются через QtConcurrent::run(),
 * результат приходит сигналом resultReady().
 */
class Calculator : public QObject
{
    Q_OBJECT

public:
    static constexpr int DefaultDelayMs = 2000;

    explicit Calculator(QObject *parent = nullptr);

    double result() const { return m_result; }
    bool hasError() const { return m_hasError; }
    QString errorMessage() const { return m_errorMessage; }

    /// Количество операций, выполняющихся в фоне.
    int pendingCount() const { return m_pendingCount; }

    /// Искусственная задержка операций, мс.
    int delayMs() const { return m_delayMs; }

public slots:
    void add(double a, double b);
    void subtract(double a, double b);
    void multiply(double a, double b);
    void divide(double a, double b);

    void reset();
    void reportError(const QString& message);
    void setDelayMs(int ms);

signals:
    /// Операция запущена в фоне.
    void operationStarted(const QString& expression);

    /// Результат готов (излучается в главном потоке).
    void resultReady(const QString& expression, double result);

    void errorOccurred(const QString &message);

    /// Завершилась последняя фоновая операция.
    void allFinished();

private:
    double m_result;
    bool m_hasError;
    QString m_errorMessage;
    int m_pendingCount;
    int m_delayMs;

    /// Запускает compute в фоновом потоке и следит за результатом через QFutureWatcher.
    void startOperation(const QString& expression, std::function<double()> compute);

    void setResult(const QString& expression, double value);
};
#endif // CALCULATOR_H
