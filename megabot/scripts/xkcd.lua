local mbutils = require( "mbutils" )
local JSON = require( "JSON" )
local sq = require( "lsqlite3" )
local sprintf = string.format
local u_dec = mbutils.url_decode
local trim = mbutils.trim
local re_match = string.match
local tolower = string.lower
local db

local upd_interval = 60000
local updating = true
local upd_idx = 0
local max_idx = 0
local announce_new = false

function format_and_send_entry( ent )
	local msg = sprintf( "XKCD: %s ( Posted on: %s, URL: %s )",
			ent.title,
			ent.time,
			sprintf( "http://xkcd.org/%d/", ent.id ) )
	send_room_message( msg )
end

function handle_network_reply( reply )
	if ( not reply.status ) then
		dout( sprintf( "Failed to retrieve data for ( %d ) '%s': %s", upd_idx, reply.url, reply.data ) )
		if ( reply.name ~= "QUERY_MAX" ) then
			upd_idx = upd_idx + 1
			local url = sprintf( "http://xkcd.org/%d/info.0.json", upd_idx )
			dout( sprintf( "Continuing update from url '%s'", url ) )
			network_request( "QUERY_INFO", url )
			return
		else
			dout( "Failed to retrieve the max idx, aborting" )
			quit()
			return
		end
	end

	local ok, data = pcall( JSON.Decode, reply.data )
	if ( not ok ) then
		dout( sprintf( "Failed to parse JSON string from url '%s', aborting", reply.url ) )
		quit()
		return
	end

	if ( reply.name == "QUERY_MAX" ) then
		local url = sprintf( "http://xkcd.org/%d/info.0.json", upd_idx )
		max_idx = tonumber( data.num )
		if ( upd_idx <= max_idx ) then
			dout( sprintf( "Got current maximum: %d. Starting update from '%s'", max_idx, url ) )
			network_request( "QUERY_INFO", url )
		else
			dout( sprintf( "Got current maximum: %d, current upd_idx = %d, no update needed", max_idx, upd_idx ) )
			set_timer( "upd_timer", upd_interval )
			announce_new = true
		end
		return
	end

	sql = db:prepare( "INSERT INTO \"xkcd\" VALUES ( ?, ?, ? )" )
	sql:bind( 1, data.num )
	sql:bind( 2, sprintf( "%02d.%02d.%d", tonumber( data.day  ), tonumber( data.month ), tonumber( data.year ) ) )
	sql:bind( 3, data.title )
	st = sql:step()
	if ( st ~= sq.DONE ) then
		if ( st == sq.CONSTRAINT ) then
			dout( sprintf( "Duplicate id %d (?)", data.num ) )
		else
			dout( sprintf( "sql:step(): %d", st ) )
		end
	end
	sql:finalize()
	if ( announce_new ) then
		format_and_send_entry( {
			['title'] = data.title,
			['time'] = sprintf( "%02d.%02d.%d", tonumber( data.day  ), tonumber( data.month ), tonumber( data.year ) ),
			['id'] = data.num
		} )
	end

	upd_idx = upd_idx + 1
	if ( upd_idx <= max_idx ) then
		local url = sprintf( "http://xkcd.org/%d/info.0.json", upd_idx )
		dout( sprintf( "Continuing update from url '%s'", url ) )
		network_request( "QUERY_INFO", url )
	else
		dout( "Update done" )
		set_timer( "upd_timer", upd_interval )
		updating = false
		announce_new = true
	end
end

function handle_timer_event( timer )
	updating = true
	dout( "Starting routine check for new comics" )
	local sql = db:prepare( "SELECT MAX( \"id\" ) AS \"mid\" FROM \"xkcd\"" )
	code = sql:step()
	if ( code ~= sq.ROW ) then
		upd_idx = 1
	else
		local tbl = sql:get_named_values()
		upd_idx = tonumber( tbl.mid )
		if ( not upd_idx ) then
			upd_idx = 0
		end
		upd_idx = upd_idx + 1
	end
	sql:finalize()
	network_request( "QUERY_MAX", "http://xkcd.com/info.0.json" )
end

function handle_room_config( config )
end

function handle_room_presence( presence )
end

function handle_room_message( message )
	local unused, cmd = re_match( message.body, "(:xkcd)%s+([^%s]+)" )
	if ( not unused ) then
		if ( trim( tolower( message.body ) ) == ":xkcd" ) then
			send_room_message( "Usage:\n  :xkcd <stats|rand[om]|latest|COMIC_ID>" )
		end
		return
	end
	dout( sprintf( "unused = \"%s\", cmd = \"%s\"", unused, cmd ) )
	if ( tolower( cmd ) == "stats" ) then
		local sql = db:prepare( "SELECT COUNT( \"id\" ) AS \"cnt\" FROM \"xkcd\"" )
		local code = sql:step()
		if ( code ~= sq.ROW ) then
			dout( sprintf( "sql:step(): %d", code ) )
			return
		end
		local tbl = sql:get_named_values()
		send_room_message( sprintf( "There are currently %d entries in the database", tbl.cnt ) )
		sql:finalize()
	elseif ( tolower( cmd ) == "latest" ) then
		local sql = db:prepare( "SELECT * FROM \"xkcd\" WHERE \"id\" = ( SELECT MAX( \"id\" ) FROM \"xkcd\" )" )
		local code = sql:step()
		if ( code ~= sq.ROW ) then
			dout( sprintf( "sql:step(): %d", code ) )
			return
		end
		local tbl = sql:get_named_values()
		format_and_send_entry( tbl )
		sql:finalize()
	elseif ( tolower( cmd ) == "rand" or tolower( cmd ) == "random" ) then
		local sql = db:prepare( "SELECT * FROM \"xkcd\" ORDER BY RANDOM() LIMIT 1" )
		local code = sql:step()
		if ( code ~= sq.ROW ) then
			dout( sprintf( "sql:step(): %d", code ) )
			return
		end
		local tbl = sql:get_named_values()
		format_and_send_entry( tbl )
		sql:finalize()
	elseif ( tonumber( cmd ) ) then
		local n = tonumber( cmd )
		local sql = db:prepare( sprintf( "SELECT * FROM \"xkcd\" WHERE \"id\" = %d", n ) )
		local code = sql:step()
		if ( code ~= sq.ROW ) then
			send_room_message( sprintf( "XKCD: Entry %d has not been indexed. Here is the URL anyway: %s",
					n, sprintf( "http://xkcd.org/%d/", n ) ) )
			return
		end
		local tbl = sql:get_named_values()
		format_and_send_entry( tbl )
		sql:finalize()
	else
		send_room_message( "Usage:\n  :xkcd <stats|rand[om]|latest|COMIC_ID>" )
	end
end

local code, str
db, code, str = sq.open( "xkcd.sqlite3" )
if ( not db ) then
	dout( sprintf( "Failed to open/create the database: ( %d ) %s", code, str ) )
	quit()
end

local st = db:exec( "CREATE TABLE IF NOT EXISTS \"xkcd\" ( \"id\" INTEGER PRIMARY KEY UNIQUE, \"time\", \"title\" )" )
if ( st ~= sq.OK ) then
	dout( sprintf( "Failed to create table: %d", st ) )
	quit()
end

local sql = db:prepare( "SELECT MAX( \"id\" ) AS \"mid\" FROM \"xkcd\"" )
code = sql:step()
if ( code ~= sq.ROW ) then
	upd_idx = 1
else
	local tbl = sql:get_named_values()
	upd_idx = tonumber( tbl.mid )
	if ( not upd_idx ) then
		upd_idx = 0
	end
	upd_idx = upd_idx + 1
end
sql:finalize()

dout( sprintf( "upd_idx = %d, starting update", upd_idx ) )
network_request( "QUERY_MAX", "http://xkcd.com/info.0.json" )
