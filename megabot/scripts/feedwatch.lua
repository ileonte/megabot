local mbutils = require( "mbutils" )
require( "feedparser" )
local sprintf = string.format
local print_r = mbutils.show_table
local sq = require( "lsqlite3" )
local db
local feeds = {}
local current_feed = 0

function handle_room_config( config )
end

function handle_room_message( message )
end

function handle_room_presence( presence )
end

function handle_timer_event( timer )
	current_feed = 1
	network_request( feeds[current_feed].name, feeds[current_feed].url )
end

function start_next_feed()
	current_feed = current_feed + 1
	if ( current_feed > #feeds ) then
		set_timer( 30000 )
	else
		network_request( feeds[current_feed].name, feeds[current_feed].url )
	end
end

function handle_network_reply( reply )
	if ( not reply.status ) then
		dout( sprintf( "Error retrieving URL '%s': %s", reply.url, reply.data ) )
		return start_next_feed()
	end

	local tbl = feedparser.parse( reply.data, reply.url )
	local i
	local upd = feeds[current_feed].time
	local upn = 0
	local msg = ""
	local cnt = 0
	local ext = 0
	for i = 1, #tbl.entries do
		local ent = tbl.entries[i]
		if ( ent.published_parsed > upd ) then
			if ( ent.published_parsed > upn ) then
				upn = ent.published_parsed
			end
			local dfmt = os.date( "%d-%m-%Y %H:%M:%S", ent.published_parsed )
			if ( msg == "" ) then
				msg = sprintf( "'%s' has the following new entries:", reply.name )
			end
			if ( cnt < 10 ) then
				msg = msg .. sprintf( "\n(*) Author: %s\n(*) Time: %s\n(*) URL: %s\n(*) Description: %s\n",
				                      ent.author,
						      dfmt,
						      ent.link,
						      ent.title )
				cnt = cnt + 1
			else
				ext = ext + 1
			end
		end
	end
	if ( msg ~= "" ) then
		feeds[current_feed].time = upn
		db:exec( sprintf( "UPDATE \"feedstatus\" SET \"time\"=%d WHERE \"id\"=%d", upn, feeds[current_feed].id ) )
		if ( ext > 0 ) then
			if ( ext == 1 ) then
				msg = msg .. sprintf( "\n\nOne other entry ommited in this report" )
			else
				msg = msg .. sprintf( "\n\n%d other entries ommited in this report", ext )
			end
		end
		send_room_message( msg )
	end

	return start_next_feed()
end

--[[
--   Script startup
--]]
local code, str
db, code, str = sq.open( "feedwatch.sqlite3" )
if ( not db ) then
	dout( string.format( "Failed to open/create the database: ( %d ) %s", code, str ) )
	quit()
end

local st = db:exec( "CREATE TABLE IF NOT EXISTS \"feedstatus\" ( \"id\" INTEGER PRIMARY KEY AUTOINCREMENT, \"name\" UNIQUE, \"url\" UNIQUE, \"time\" INTEGER )" )
if ( st ~= sq.OK ) then
	dout( string.format( "Failed to create table: %d", st ) )
	quit()
end

sql = db:prepare( "SELECT * FROM \"feedstatus\"" )
st = sql:step()
if ( st ~= sq.ROW ) then
	dout( string.format( "sql:step(): %d", st ) )
	sql:finalize()
	quit()
else
	local tbl = sql:get_named_values()
	table.insert( feeds, tbl )
end
sql:finalize()

if ( #feeds == 0 ) then
	dout( "No feed definitions loaded, quitting" );
	quit()
end

current_feed = 1
network_request( feeds[current_feed].name, feeds[current_feed].url )
