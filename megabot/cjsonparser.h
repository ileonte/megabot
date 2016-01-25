#ifndef CJSONPARSER_H
#define CJSONPARSER_H

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QFile>
#include <QString>

typedef QVector<QVariant> QVariantArray;

class CJSONParser : public QObject
{
	Q_OBJECT

public:
	typedef struct {
		qint64  line;
		qint64  column;
		QString message;
	} ParserError;

private:
	qint64 m_line;
	qint64 m_column;

	ParserError m_error;

	QVariant m_value;

	uchar  *m_map;
	qint64  m_max;
	qint64  m_idx;
	int     m_inObj;
	int     m_inArr;
	int     m_inCom;

	void setError( int l, int c, const QString &m ) {
		m_error.line    = l;
		m_error.column  = c;
		m_error.message = m;
	}

	int popChar( uchar &ch );
	void forward( int c = 1 );

	int skipWhitespace();

	int parseValue( QVariant &val );
	int parseObject( QVariantMap &obj );
	int parseArray( QVariantList &arr );
	int parseString( QString &str );
	int parseNumber( QVariant &val );

public:
	explicit CJSONParser( QObject *parent = 0 );
	virtual ~CJSONParser() {}

	void clear();
	ParserError error() const { return m_error; }
	bool valid() const {
		return m_value.type() == QVariant::Map || m_value.type() == QVariant::List || m_value.type() == QVariant::String ||
		       m_value.type() == QVariant::LongLong || m_value.type() == QVariant::Double;
	}

	bool parse( const QString &fileName );

	QVariant::Type type() const { return m_value.type(); }
	const QVariant &value() const { return m_value; }
};

#endif // CJSONPARSER_H
