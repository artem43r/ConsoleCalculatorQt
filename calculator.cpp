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

    auto* watcher = new QFutureWatcher<double>(this);

    // Подключаемся до setFuture, чтобы не пропустить завершение
    connect(watcher, &QFutureWatcher<double>::finished, this,
            [this, watcher, expression]() {
                const double value = watcher->result();
                watcher->deleteLater();

                --m_pendingCount;
                setResult(expression, value);

                if (m_pendingCount == 0)
                    emit allFinished();
            });

    // Лямбда выполняется в другом потоке, поэтому получает копии данных, а не поля класса
    const int delay = m_delayMs;

    watcher->setFuture(QtConcurrent::run([compute, delay, expression]() {
        qDebug() << "compute" << expression
                 << "in thread" << QThread::currentThreadId();
        QThread::msleep(delay);  // имитация долгого вычисления
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
