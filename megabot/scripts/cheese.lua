local mbutils = require( "mbutils" )
local sq = require( "lsqlite3" )
local trim = mbutils.trim
local db

function handle_room_config( config )
end

function handle_room_message( message )
	local cmd, arg
	local st, code, str
	local sql

	function simple_query( qry )
		sql = db:prepare( qry )
		st = sql:step()
		if ( st == sq.DONE ) then
			sql:finalize()
			return true
		end
		if ( st ~= sq.ROW ) then
			sql:finalize()
			return nil
		end
		local tbl = sql:get_named_values()
		sql:finalize()
		return tbl
	end

	cmd, arg = string.match( message.body, "(:savecheese)%s+(.*)" )
	if ( cmd and arg ) then
		arg = trim( arg )
		if ( arg == '' ) then
			return
		end
		sql = db:prepare( "INSERT INTO \"quotes\" VALUES ( NULL, ?, ?, ?, ? )" )
		sql:bind( 1, message.localTime )
		sql:bind( 2, message.serverTime )
		sql:bind( 3, message.nickName )
		sql:bind_blob( 4, arg )
		st = sql:step()
		if ( st ~= sq.DONE ) then
			if ( st == sq.CONSTRAINT ) then
				send_room_message( string.format( "%s: A MAI FOST !!1", message.nickName ) )
			end
			dout( string.format( "sql:step(): %d", st ) )
		end
		sql:finalize()
		return
	end

	cmd, arg = string.match( message.body, "(:expirecheese)%s+(.*)" )
	if ( cmd and arg ) then
		arg = trim( arg )
		if ( arg == '' ) then
			return
		end
		sql = db:prepare( "DELETE FROM \"quotes\" WHERE \"quote\" = ?" )
		sql:bind_blob( 1, arg )
		st = sql:step()
		dout( string.format( "Delete: %d", st ) )
		sql:finalize()
		return
	end

	cmd = string.match( message.body, "(:cheese)%s*$" )
	if ( cmd ) then
		sql = db:prepare( "SELECT * FROM \"quotes\" ORDER BY RANDOM() LIMIT 1" )
		st = sql:step()
		if ( st ~= sq.ROW ) then
			dout( string.format( "sql:step(): %d", st ) )
		else
			local tbl = sql:get_named_values()
			local msg = string.format( "\n%s\n---\nSubmitted by \"%s\" on %s", tbl.quote, tbl.submitter, os.date( "%a, %d %b %Y %H:%M:%S %z", tbl.localtime ) )
			send_room_message( msg )
		end
		sql:finalize()
		return
	end

	cmd = string.match( message.body, "(:cheesestats)%s*$" )
	if ( cmd ) then
		local tbl = simple_query( "SELECT COUNT( \"id\" ) AS \"count\" FROM \"quotes\" WHERE 1" )
		local count = tbl.count
		local top = {}
		local msg

		sql = db:prepare( "SELECT \"submitter\" AS \"name\", COUNT( \"submitter\" ) AS \"count\" FROM \"quotes\" GROUP BY \"submitter\" ORDER BY \"count\" DESC LIMIT 3" )
		st = sql:step()
		while ( st == sq.ROW ) do
			local tbl = sql:get_named_values()
			table.insert( top, tbl )
			st = sql:step()
		end
		sql:finalize()

		if ( count == 0 ) then
			msg = "Alas, the dairy queen doth not smile upon us ! No cheese is to be found in our pantry ! OH, WOE IS US !"
		else
			if ( count == 1 ) then
				msg = "\nOne piece of cheese is currently stored in our pantry\n"
			else
				msg = string.format( "\nThere are currently %d pieces of cheese in our pantry\n", count )
			end
			if ( #top == 1 ) then
				msg = msg .. string.format( "Our top (and only) dairy technician thus far is \"%s\".", top[1].name )
			else
				msg = msg .. string.format( "Our top %d dairy technicians are:\n", #top )
				for i, v in ipairs( top ) do
					msg = msg .. string.format( "   \"%s\" with %d piece(s) of cheese produced\n", v.name, v.count )
				end
			end
			msg = msg .. "\nThe milk must flow !"
		end

		send_room_message( msg )

		return
	end
end

function handle_room_presence( presence )
end

function handle_shutdown()
	if ( db ) then
		db:close()
	end
end

local code, str
db, code, str = sq.open( "cheesedb.sqlite3" )
if ( not db ) then
	dout( string.format( "Failed to open/create the database: ( %d ) %s", code, str ) )
	quit()
end

local st = db:exec( "CREATE TABLE IF NOT EXISTS \"quotes\" ( \"id\" INTEGER PRIMARY KEY AUTOINCREMENT, \"localtime\", \"servertime\", \"submitter\", \"quote\" UNIQUE )" )
if ( st ~= sq.OK ) then
	dout( string.format( "Failed to create table: %d", st ) )
	quit()
end

local f = io.open( "cheese.db" )
if ( not f ) then
	dout( "No old DB to import" )
else
	for msg in f:lines() do
		local sql = db:prepare( "INSERT INTO \"quotes\" VALUES ( NULL, ?, ?, ?, ? )" )
		sql:bind( 1, os.time() )
		sql:bind( 2, os.time() )
		sql:bind( 3, "MegaBot v1" )
		sql:bind_blob( 4, msg )
		st = sql:step()
		sql:finalize()
	end
	f:close()
end

