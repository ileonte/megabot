#include <QDebug>
#include <QByteArray>

#include <ctype.h>

#include "cjsonparser.h"

#define CH_EOF -1
#define CH_CHR  0

#define CP_ERR -1
#define CP_TOK  0
#define CP_EOF  1

void CJSONParser::clear()
{
	m_line   = 1;
	m_column = 1;

	m_error.line = m_error.column = -1;
	m_error.message = QString();

	m_value.clear();
}

CJSONParser::CJSONParser( QObject *parent ) : QObject( parent )
{
	clear();
}

bool CJSONParser::parse( const QString &fileName )
{
	QFile src( fileName );
	bool  ret = false;

	clear();

	if ( !src.open( QIODevice::ReadOnly ) ) {
		setError( -1, -1, src.errorString() );
		return false;
	}

	if ( !src.size() ) {
		setError( -1, -1, "The file is empty" );
		return false;
	}

	uchar *srcmap = src.map( 0, src.size() );
	if ( !srcmap ) {
		setError( -1, -1, src.errorString() );
		return false;
	}

	m_map   = srcmap;
	m_idx   = 0;
	m_max   = src.size();
	m_inObj = 0;
	m_inArr = 0;
	m_inCom = 0;

	if ( skipWhitespace() == CP_TOK ) {
		ret = true;
		if ( parseValue( m_value ) < 0 )
			ret = false;
	} else {
		setError( m_line, m_column, "End-of-file reached before any parsable value was found" );
		ret = false;
	}

	src.unmap( srcmap );
	src.close();

	return ret;
}

int CJSONParser::parseValue( QVariant &val )
{
	uchar ch = 0;

	popChar( ch );
	switch ( ch ) {
		case '"': {
			QString str;

			forward();
			if ( parseString( str ) < 0 ) return -1;
			else val = str;

			break;
		}
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': {
			if ( parseNumber( val ) < 0 ) return -1;

			break;
		}
		case '{': {
			QVariantMap obj;

			forward();
			if ( parseObject( obj ) < 0 ) return -1;
			else val = obj;

			break;
		}
		case '[': {
			QVariantList arr;

			forward();
			if ( parseArray( arr ) < 0 ) return -1;
			else val = arr;

			break;
		}
		case 't': {
			if ( m_idx > m_max - 4 ) {
				setError( m_line, m_column, "Unexpected character encountered: 't'" );
				return -1;
			}
			if ( !memcmp( "true", m_map + m_idx, 4 ) ) {
				if ( m_idx == m_max - 4 ) {
					val = true;
				} else {
					if ( isalnum( m_map[m_idx + 4] ) ) {
						setError( m_line, m_column, "Unexpected character encountered: 't'" );
						return -1;
					}
					val = true;
				}
				forward( 4 );
			} else {
				setError( m_line, m_column, "Unexpected character encountered: 't'" );
				return -1;
			}
			break;
		}
		case 'f': {
			if ( m_idx > m_max - 5 ) {
				setError( m_line, m_column, "Unexpected character encountered: 'f'" );
				return -1;
			}
			if ( !memcmp( "false", m_map + m_idx, 5 ) ) {
				if ( m_idx == m_max - 5 ) {
					val = false;
				} else {
					if ( isalnum( m_map[m_idx + 5] ) ) {
						setError( m_line, m_column, "Unexpected character encountered: 'f'" );
						return -1;
					}
					val = false;
				}
				forward( 5 );
			} else {
				setError( m_line, m_column, "Unexpected character encountered: 'f'" );
				return -1;
			}
			break;
		}
		case 'n': {
			if ( m_idx > m_max - 4 ) {
				setError( m_line, m_column, "Unexpected character encountered: 'n'" );
				return -1;
			}
			if ( !memcmp( "null", m_map + m_idx, 4 ) ) {
				if ( m_idx == m_max - 4 ) {
					val.clear();
				} else {
					if ( isalnum( m_map[m_idx + 4] ) ) {
						setError( m_line, m_column, "Unexpected character encountered: 'n'" );
						return -1;
					}
					val.clear();
				}
				forward( 4 );
			} else {
				setError( m_line, m_column, "Unexpected character encountered: 'n'" );
				return -1;
			}
			break;
		}
		default: {
			setError( m_line, m_column, QString( "Unexpected character encountered: '%1'" ).arg( QChar( ch ) ) );
			return -1;
		}
	}

	return 0;
}

int CJSONParser::popChar( uchar &ch )
{
	if ( m_idx < m_max ) {
		ch = m_map[m_idx];
		return CH_CHR;
	} else {
		return CH_EOF;
	}
}

void CJSONParser::forward( int c )
{
	if ( m_idx < m_max ) {
		if ( m_map[m_idx] == '\n' ) {
			m_column = 1;
			m_line++;
		} else {
			m_column += c;
		}
		m_idx += c;
	}
}

int CJSONParser::skipWhitespace()
{
	uchar ch;

	while ( 1 ) {
		if ( popChar( ch ) == CH_EOF )
			return CP_EOF;

		switch ( ch ) {
			case '#': {
				m_inCom = 1;
				break;
			}
			case '\n': {
				m_inCom = 0;
				break;
			}
			case ' ':
			case '\t': {
				break;
			}
			default: {
				if ( !m_inCom ) return CP_TOK;
				break;
			}
		}

		forward();
	}
}

int CJSONParser::parseString( QString &str )
{
	uchar   ch;
	bool    esc = false;

	str = "";

	while ( 1 ) {
		if ( popChar( ch ) == CH_EOF ) {
			setError( m_line, m_column, "End-of-file encountered while parsing string value" );
			return -1;
		}
		forward();

		if ( esc ) {
			switch ( ch ) {
				case '\\':
				case '/':
				case '"': {
					str += ch;
					break;
				}
				case 'n': {
					str += '\n';
					break;
				}
				case 'r': {
					str += '\r';
					break;
				}
				case 't': {
					str += '\t';
					break;
				}
				case 'b': {
					str += '\n';
					break;
				}
				case 'f': {
					str += '\f';
					break;
				}
				case 'u': {
					uchar d[4] = {};
					bool ok = false;
					quint16 v = 0;

					if ( m_idx > m_max - 4 ) {
						setError( m_line, m_column, "Truncated UTF sequence" );
						return -1;
					}

					memcpy( d, m_map + m_idx, 4 );

					QString s( QByteArray( ( const char * )d, 4 ) );
					v = s.toUShort( &ok, 16 );
					if ( !ok ) {
						setError( m_line, m_column, QString( "Invalid UTF code point '%1'" ).arg( s ) );
						return -1;
					}

					str += QChar( v );

					forward( 4 );

					break;
				}
				default: {
					break;
				}
			}
			esc = 0;
		} else {
			if ( ch == '"' ) return 0;
			else if ( ch == '\\' ) esc = true;
			else str += ch;
		}
	}
}

int CJSONParser::parseObject( QVariantMap &obj )
{
	int cc = 0;

	while ( 1 ) {
		QString  key;
		QVariant val;
		uchar    ch = 0;

		/* key */
		if ( skipWhitespace() != CP_TOK ) {
			setError( m_line, m_column, "End-of-file encountered while looking for key name" );
			return -1;
		}
		popChar( ch );
		if ( ch != '"' ) {
			if ( ch == ',' ) {
				cc++;
				if ( cc == 1 ) {
					forward();
					continue;
				} else {
					setError( m_line, m_column, "Expected string as key value, found ','" );
					return -1;
				}
			} else if ( ch == '}' ) {
				forward();
				return 0;
			} else {
				setError( m_line, m_column, QString( "Expected string as key value, found '%1'" ).arg( QChar( ch ) ) );
				return -1;
			}
		}
		forward();
		if ( parseString( key ) < 0 ) return -1;
		cc = 0;

		/* separator */
		if ( skipWhitespace() != CP_TOK ) {
			setError( m_line, m_column, "End-of-file encountered while looking for key-value separator" );
			return -1;
		}
		popChar( ch );
		if ( ch != ':' ) {
			setError( m_line, m_column, QString( "Expecting ':', found '%1' instead" ).arg( QChar( ch ) ) );
			return -1;
		}
		forward();

		/* value */
		if ( skipWhitespace() != CP_TOK ) {
			setError( m_line, m_column, "End-of-file encountered while looking for value" );
			return -1;
		}
		if ( parseValue( val ) < 0 ) return -1;

		obj[key] = val;
	}

	return 0;
}

int CJSONParser::parseArray( QVariantList &arr )
{
	while ( 1 ) {
		QVariant val;
		uchar    ch = 0;

		/* separator */
		if ( skipWhitespace() != CP_TOK ) {
			setError( m_line, m_column, "End-of-file encountered while looking for key name" );
			return -1;
		}
		popChar( ch );
		if ( ch == ',' ) {
			forward();
			continue;
		}
		if ( ch == ']' ) {
			forward();
			return 0;
		}

		/* value */
		if ( skipWhitespace() != CP_TOK ) {
			setError( m_line, m_column, "End-of-file encountered while looking for value" );
			return -1;
		}
		if ( parseValue( val ) < 0 ) return -1;
		arr.push_back( val );
	}

	return 0;
}

int CJSONParser::parseNumber( QVariant &val )
{
	int    dc   = 0;
	qint64 idx  = m_idx;
	bool   done = false;
	bool   ok   = 0;
	qint64 vint = 0;
	double vdbl = 0;

	while ( !done && idx < m_max ) {
		switch ( m_map[idx] ) {
			case '.':
			case '+':
			case '-':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9': {
				if ( m_map[idx] == '.' ) dc++;
				idx++;
				break;
			}
			default: {
				done = true;
				break;
			}
		}
	}

	QString s( QByteArray( ( const char * )( m_map + m_idx ), idx - m_idx ) );
	if ( s.isEmpty() ) {
		setError( m_line, m_column, "Invalid numeric value" );
		return -1;
	}

	if ( dc > 1 ) {
		setError( m_line, m_column, QString( "Invalid numeric value: '%1'" ).arg( s ) );
		return -1;
	}

	if ( dc ) {
		vdbl = s.toDouble( &ok );
		if ( !ok ) {
			setError( m_line, m_column, QString( "Invalid floating point value: '%1'" ).arg( s ) );
			return -1;
		}
		val = vdbl;
	} else {
		vint = s.toLongLong( &ok );
		if ( !ok ) {
			setError( m_line, m_column, QString( "Invalid integer value: '%1'" ).arg( s ) );
			return -1;
		}
		val = vint;
	}

	m_column += ( idx - m_idx );
	m_idx = idx;

	return 0;
}
