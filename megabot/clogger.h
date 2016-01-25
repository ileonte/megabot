#ifndef CLOGGER_H
#define CLOGGER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QString>

void mb_trace(QObject *obj, const QString &message);

#define LOG(__s) ({ mb_trace(this, __s); })
#define OLOG(__o, __s) ({ mb_trace(__o, __s); })

#endif // CLOGGER_H
