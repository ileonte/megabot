#ifndef CLOGGER_H
#define CLOGGER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QString>

void mb_trace( QObject *obj, int line, const QString &file, const QString &message );

#define LOG( __s ) ({ mb_trace( this, __LINE__, __FILE__, __s ); 0; })
#define OLOG( __o, __s ) ({ mb_trace( __o, __LINE__, __FILE__, __s ); 0; })

#endif // CLOGGER_H
