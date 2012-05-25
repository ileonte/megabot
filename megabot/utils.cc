#include "utils.h"

namespace Utils {

	void str_break( const QString &str, QStringList &pieces )
	{
		int   start = 0;
		int   end   = 0;
		QChar qch   = 0;
		int   size  = str.size();
		bool  inq   = false;
		bool  esc   = false;
		bool  done  = false;

		pieces.clear();

		while ( 1 ) {
			QString piece = "";

			inq = false;
			esc = false;
			qch = 0;

			if ( start >= size ) goto out_of_str_break;

			while ( str.at( start ).isSpace() ) {
				start += 1;
				if ( start >= size ) goto out_of_str_break;
			}

			if ( str.at( start ) == '\'' || str.at( start ) == '"' ) {
				inq = true;
				qch = str.at( start );
				start += 1;
			}

			done = false;
			end  = start;
			while ( !done ) {

				if ( end >= size ) {
					pieces.push_back( piece );
					goto out_of_str_break;
				}

				switch ( str.at( end ).toAscii() ) {
					case '\'':
					case '"': {
						if ( esc ) {
							piece += str.at( end );
							end += 1;
							esc = false;
						} else {
							if ( inq ) {
								if ( str.at( end ) == qch ) {
									if ( esc ) {
										esc = false;
										piece += qch;
									} else {
										inq = false;
									}
									end += 1;
								} else {
									piece += str.at( end );
									end += 1;
								}
							} else {
								inq = true;
								qch = str.at( end );
								end += 1;
							}
						}
						break;
					}

					case '\\': {
						if ( esc ) {
							piece += '\\';
							esc = false;
						} else {
							esc = true;
						}
						end += 1;
						break;
					}

					case ' ': {
						if ( inq ) {
							piece += str.at( end );
							end += 1;
						} else {
							if ( esc ) {
								piece += str.at( end );
								end += 1;
								esc = false;
							} else {
								done  = true;
								start = end + 1;
								pieces.push_back( piece );
								continue;
							}
						}
						break;
					}

					default: {
						if ( esc ) {
							esc = false;
							switch ( str.at( end ).toAscii() ) {
								case 'n': {
									piece += '\n';
									break;
								}
								case 't': {
									piece += '\t';
									break;
								}
								default: {
									piece += str.at( end );
									break;
								}
							}
						} else {
							piece += str.at( end );
						}
						end += 1;
						break;
					}
				}
			}

		}

		out_of_str_break: ;
	}

}
