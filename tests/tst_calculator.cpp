#include <QtTest>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QThread>
#include "calculator.h"

/**
 * @brief Юнит-тесты асинхронного калькулятора.
 *
 * QSignalSpy записывает каждое излучение сигнала вместе с аргументами.
 * spy.wait() крутит цикл событий, пока сигнал не придёт (или не истечёт таймаут),
 * поэтому им удобно дожидаться результатов из фоновых потоков.
 */
class TestCalculator : public QObject
{
    Q_OBJECT

private:
    // Короткая задержка, чтобы тесты шли быстро, но оставались асинхронными
    static constexpr int TestDelayMs = 50;
    static constexpr int WaitTimeoutMs = 5000;

private slots:
    void operations_data();
    void operations();
    void resultIsAsynchronous();
    void divisionByZeroEmitsError();
    void errorIsClearedAfterSuccess();
    void reset();
    void pendingCountAndAllFinished();
    void operationsRunInParallel();
    void negativeDelayIsRejected();
};

// Таблица данных: один тест operations() прогоняется для каждой строки
void TestCalculator::operations_data()
{
    QTest::addColumn<QString>("operation");
    QTest::addColumn<double>("a");
    QTest::addColumn<double>("b");
    QTest::addColumn<QString>("expression");
    QTest::addColumn<double>("expected");

    QTest::newRow("add")          << "add" << 5.0  << 3.0  << "5 + 3"    << 8.0;
    QTest::newRow("add negative") << "add" << -2.5 << 1.0  << "-2.5 + 1" << -1.5;
    QTest::newRow("subtract")     << "sub" << 10.0 << 4.0  << "10 - 4"   << 6.0;
    QTest::newRow("multiply")     << "mul" << 4.0  << 2.5  << "4 * 2.5"  << 10.0;
    QTest::newRow("divide")       << "div" << 7.0  << 2.0  << "7 / 2"    << 3.5;
}

void TestCalculator::operations()
{
    QFETCH(QString, operation);
    QFETCH(double, a);
    QFETCH(double, b);
    QFETCH(QString, expression);
    QFETCH(double, expected);

    Calculator calc;
    calc.setDelayMs(TestDelayMs);
    QSignalSpy startedSpy(&calc, &Calculator::operationStarted);
    QSignalSpy resultSpy(&calc, &Calculator::resultReady);

    if (operation == "add")
        calc.add(a, b);
    else if (operation == "sub")
        calc.subtract(a, b);
    else if (operation == "mul")
        calc.multiply(a, b);
    else
        calc.divide(a, b);

    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(startedSpy.at(0).at(0).toString(), expression);

    QVERIFY(resultSpy.wait(WaitTimeoutMs));
    QCOMPARE(resultSpy.count(), 1);
    QCOMPARE(resultSpy.at(0).at(0).toString(), expression);
    QCOMPARE(resultSpy.at(0).at(1).toDouble(), expected);
    QCOMPARE(calc.result(), expected);
    QVERIFY(!calc.hasError());
}

// Слот возвращает управление до того, как результат готов
void TestCalculator::resultIsAsynchronous()
{
    Calculator calc;
    calc.setDelayMs(TestDelayMs);
    QSignalSpy resultSpy(&calc, &Calculator::resultReady);

    calc.add(1, 2);

    QCOMPARE(resultSpy.count(), 0);      // сразу после вызова результата ещё нет
    QCOMPARE(calc.pendingCount(), 1);
    QVERIFY(resultSpy.wait(WaitTimeoutMs));
    QCOMPARE(resultSpy.count(), 1);
}

void TestCalculator::divisionByZeroEmitsError()
{
    Calculator calc;
    calc.setDelayMs(TestDelayMs);
    QSignalSpy resultSpy(&calc, &Calculator::resultReady);
    QSignalSpy errorSpy(&calc, &Calculator::errorOccurred);

    calc.divide(1, 0);

    // Ошибка сообщается сразу, фоновая задача не запускается
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.at(0).at(0).toString(), QString("Деление на ноль невозможно!"));
    QCOMPARE(calc.pendingCount(), 0);
    QVERIFY(calc.hasError());

    // И результат так и не приходит
    QVERIFY(!resultSpy.wait(TestDelayMs * 4));
    QCOMPARE(resultSpy.count(), 0);
}

void TestCalculator::errorIsClearedAfterSuccess()
{
    Calculator calc;
    calc.setDelayMs(TestDelayMs);
    QSignalSpy resultSpy(&calc, &Calculator::resultReady);

    calc.divide(1, 0);
    QVERIFY(calc.hasError());

    calc.add(2, 2);
    QVERIFY(resultSpy.wait(WaitTimeoutMs));
    QVERIFY(!calc.hasError());
    QVERIFY(calc.errorMessage().isEmpty());
}

void TestCalculator::reset()
{
    Calculator calc;
    calc.setDelayMs(TestDelayMs);
    QSignalSpy resultSpy(&calc, &Calculator::resultReady);

    calc.multiply(3, 3);
    QVERIFY(resultSpy.wait(WaitTimeoutMs));
    QCOMPARE(calc.result(), 9.0);

    calc.reset();

    // reset синхронный: сигнал приходит сразу
    QCOMPARE(resultSpy.count(), 2);
    QCOMPARE(resultSpy.at(1).at(1).toDouble(), 0.0);
    QCOMPARE(calc.result(), 0.0);
}

void TestCalculator::pendingCountAndAllFinished()
{
    Calculator calc;
    calc.setDelayMs(TestDelayMs);
    QSignalSpy resultSpy(&calc, &Calculator::resultReady);
    QSignalSpy finishedSpy(&calc, &Calculator::allFinished);

    calc.add(1, 1);
    calc.add(2, 2);
    calc.add(3, 3);
    QCOMPARE(calc.pendingCount(), 3);

    QVERIFY(finishedSpy.wait(WaitTimeoutMs));
    QCOMPARE(finishedSpy.count(), 1);    // ровно один раз, после последней операции
    QCOMPARE(resultSpy.count(), 3);
    QCOMPARE(calc.pendingCount(), 0);
}

// Три операции по 300 мс параллельно должны занять заметно меньше 900 мс
void TestCalculator::operationsRunInParallel()
{
    if (QThread::idealThreadCount() < 3)
        QSKIP("Для проверки параллельности нужно хотя бы 3 ядра");

    const int delay = 300;
    Calculator calc;
    calc.setDelayMs(delay);
    QSignalSpy finishedSpy(&calc, &Calculator::allFinished);

    QElapsedTimer timer;
    timer.start();

    calc.add(1, 1);
    calc.subtract(1, 1);
    calc.multiply(1, 1);

    QVERIFY(finishedSpy.wait(WaitTimeoutMs));
    QVERIFY2(timer.elapsed() < delay * 2,
             qPrintable(QString("Прошло %1 мс").arg(timer.elapsed())));
}

void TestCalculator::negativeDelayIsRejected()
{
    Calculator calc;
    QSignalSpy errorSpy(&calc, &Calculator::errorOccurred);

    calc.setDelayMs(-1);

    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(calc.delayMs(), Calculator::DefaultDelayMs);
}

QTEST_GUILESS_MAIN(TestCalculator)
#include "tst_calculator.moc"
