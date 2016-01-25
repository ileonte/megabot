#ifndef UTILS_H
#define UTILS_H

#include <QChar>
#include <QString>
#include <QMap>
#include <QStringList>

typedef QMap<QString, QString> QStringMap;

namespace Utils {
	void str_break( const QString &str, QStringList &pieces );
}

#endif // UTILS_H
