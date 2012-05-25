require( "mbutils" )
require( "feedparser" )
local sq = require( "lsqlite3" )
local db

function handle_timer_event( timer )
	send_room_message( string.format( "Timer '%s' has ticked", timer ) )
end

function handle_room_config( config )
end

function handle_room_message( message )
	if ( message.body == ":test" ) then
		local t = {
			["test"] = "Test string",
			['tab']  = {
				['number'] = 10.10,
				['int']    = 5,
				['string'] = 'string'
			},
			['bool'] = true
		}
		local s = serialize( t )
		local b, r = unserialize( s )
		send_room_message( string.format( "Result: %s", mbutils.show_table( r ) ) )
	end
end

function handle_room_presence( presence )
end

math.randomseed( os.time() )
dout( string.format( "Script started - '%s'", sq.version() ) )
