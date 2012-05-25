require( "mbutils" )

local re_match = string.match
local sprintf = string.format
local rand = math.random
local srand = math.randomseed
local tinsert = table.insert
local implode = table.concat

function handle_room_config( config )
end

function do_roll( who, r_min, r_max, r_cnt )
	msg = ""
	sum = 0

	if ( r_cnt == 1 ) then
		msg = sprintf( "%s has rolled %d", who, rand( r_min, r_max ) )
	else
		i = 0
		r = 0
		rolls = {}

		for i = 1, r_cnt do
			r = rand( r_min, r_max )
			sum = sum + r
			tinsert( rolls, r )
		end
		msg = sprintf( "%s has rolled %d - individual rolls: %s", who, sum, implode( rolls, " " ) )
	end
	send_room_message( msg )
end

function roll_me_a( r_tar )
	r_cnt = 0
	r = 0
	while ( r ~= r_tar ) do
		r_cnt = r_cnt + 1
		r = rand( 1, 100 )
	end

	return r_cnt
end

function handle_room_message( message )
	r_min = 1
	r_max = 100
	r_cnt = 0
	r_tar = 0
	r_cmd = ""

	r_cmd, r_min, r_max = re_match( message.body, "(:roll)%s+(%d+)-(%d+)" )
	if ( r_cmd and r_min and r_max ) then
		r_min = tonumber( r_min )
		r_max = tonumber( r_max )
		if ( r_min < 1 ) then
			r_min = 1
		end
		if ( r_max < r_min ) then
			r_max = r_min
		end
		r_cnt = 1

		do_roll( message.nickName, r_min, r_max, r_cnt )

		return
	end
	r_cmd, r_cnt, r_max = re_match( message.body, "(:roll)%s+(%d+)d(%d+)" )
	if ( r_cmd and r_cnt and r_max ) then
		r_cnt = tonumber( r_cnt )
		r_max = tonumber( r_max )
		if ( r_max < 3 ) then
			r_max = 3
		end
		if ( r_max > 100 ) then
			r_max = 100
		end
		if ( r_cnt < 1 ) then
			r_cnt = 1
		end
		r_min = 1

		do_roll( message.nickName, r_min, r_max, r_cnt )

		return
	end
	r_cmd = re_match( message.body, "(:roll)%s*$" )
	if ( r_cmd ) then
		r_cnt = 1
		r_min = 1
		r_max = 100

		do_roll( message.nickName, r_min, r_max, r_cnt )

		return
	end

	r_cmd, r_tar = re_match( message.body, "(:roll%s+me%s+a)%s+(%d+)" )
	if ( r_cmd and r_tar ) then
		r_tar = tonumber( r_tar )
		if ( r_tar < 1 ) then
			r_tar = 50
		end
		if ( r_tar > 100 ) then
			r_tar = 100
		end

		r_cnt = roll_me_a( r_tar )

		send_room_message( sprintf( "%s has rolled a %d after %d attempt(s)", message.nickName, r_tar, r_cnt ) )

		return
	end

	r_cmd, r_tar = re_match( message.body, "(:rollercoaster%s+from)%s+(%d+)" )
	if ( r_cmd and r_tar ) then
		r_tar = tonumber( r_tar )
		if ( r_tar < 1 ) then
			r_tar = 50
		end
		if ( r_tar > 100 ) then
			r_tar = 100
		end

		done = false
		ride_count = 0
		while ( not done ) do
			ride_count = ride_count + 1
			r_cnt = roll_me_a( r_tar )
			send_room_message( sprintf( "%s has rolled a %d after %d attempt(s)", message.nickName, r_tar, r_cnt ) )
			if ( r_cnt <= 100 ) then
				r_tar = r_cnt
			else
				done = true
			end
		end
		send_room_message( sprintf( "%s has been kicked off the :rollercoaster after riding it %d time(s)", message.nickName, ride_count ) )

		return
	end
end

function handle_room_presence( presence )
end

srand( os.time() )
