#ifndef PATH_H
#define PATH_H

#include <QString>
#include <QList>

struct PathPoint
{
    double x;
    double y;
    double speed;

public:
    double getY() const;
    void setY(double value);
    double getX() const;
    void setX(double value);
    double getSpeed() const;
    void setSpeed(double value);
};

class Path :public QList<PathPoint>
{
public:
    Path();
private:
    QString name;
    bool m_closed, m_smooth;
    int m_precision;
};
#endif // PATH_H
