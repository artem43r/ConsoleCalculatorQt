#ifndef CALCULATOR_H
#define CALCULATOR_H
#include <QObject>
#include <QString>

class Calculator : public QObject
{
    Q_OBJECT

public:
    explicit Calculator(QObject *parent = nullptr);

    double result() const { return m_result; }
    bool hasError() const { return m_hasError; }
    QString errorMessage() const { return m_errorMessage; }

public slots:
    void add(double a, double b);
    void subtract(double a, double b);
    void multiply(double a, double b);
    void divide(double a, double b);

    void reset();
    void reportError(const QString& message);

signals:
    void resultReady(double result);
    void errorOccurred(const QString &message);

private:
    double m_result;
    bool m_hasError;
    QString m_errorMessage;

    void setResult(double value);
};
#endif // CALCULATOR_H