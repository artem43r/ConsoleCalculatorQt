#include "calculator.h"
#include <QDebug>
#include <QFutureWatcher>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

Calculator::Calculator(QObject* parent)
    : QObject(parent)
    , m_result(0.0)
    , m_hasError(false)
    , m_pendingCount(0)
    , m_delayMs(DefaultDelayMs)
{
    qDebug() << "Calculator created";
}

void Calculator::startOperation(const QString& expression, std::function<double()> compute)
{
    ++m_pendingCount;
    emit operationStarted(expression);

    // Наблюдатель живёт в главном потоке (родитель - калькулятор),
    // поэтому его сигнал finished обрабатывается в главном потоке.
    auto* watcher = new QFutureWatcher<double>(this);

    // Подключаемся ДО setFuture, чтобы не пропустить завершение
    // очень быстрой задачи.
    connect(watcher, &QFutureWatcher<double>::finished, this,
            [this, watcher, expression]() {
                const double value = watcher->result();
                watcher->deleteLater();  // наблюдатель больше не нужен

                --m_pendingCount;
                setResult(expression, value);

                if (m_pendingCount == 0)
                    emit allFinished();
            });

    // Копируем задержку в локальную переменную: лямбда ниже работает
    // в другом потоке и не должна читать поля калькулятора.
    const int delay = m_delayMs;

    watcher->setFuture(QtConcurrent::run([compute, delay, expression]() {
        qDebug() << "Вычисление" << expression
                 << "в потоке" << QThread::currentThreadId();
        QThread::msleep(delay);  // имитация тяжёлого вычисления
        return compute();
    }));
}

void Calculator::setResult(const QString& expression, double value)
{
    m_result = value;
    m_hasError = false;
    m_errorMessage.clear();

    emit resultReady(expression, m_result);
}

void Calculator::add(double a, double b)
{
    startOperation(QString("%1 + %2").arg(a).arg(b), [a, b]() { return a + b; });
}

void Calculator::subtract(double a, double b)
{
    startOperation(QString("%1 - %2").arg(a).arg(b), [a, b]() { return a - b; });
}

void Calculator::multiply(double a, double b)
{
    startOperation(QString("%1 * %2").arg(a).arg(b), [a, b]() { return a * b; });
}

void Calculator::divide(double a, double b)
{
    // Проверяем сразу, в главном потоке: запускать фоновую задачу,
    // которая заведомо закончится ошибкой, незачем.
    if (qFuzzyIsNull(b)) {
        reportError("Деление на ноль невозможно!");
        return;
    }

    startOperation(QString("%1 / %2").arg(a).arg(b), [a, b]() { return a / b; });
}

void Calculator::reset()
{
    qDebug() << "reset()";
    setResult("reset", 0.0);
}

void Calculator::reportError(const QString& message)
{
    m_hasError = true;
    m_errorMessage = message;

    emit errorOccurred(m_errorMessage);
}

void Calculator::setDelayMs(int ms)
{
    if (ms < 0) {
        reportError("Задержка не может быть отрицательной");
        return;
    }

    m_delayMs = ms;
}
