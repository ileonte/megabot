require( "mbutils" )
require( "JSON" )

local fight_active = false
local fight_terms = {}
local fight_idx = 0

function handle_network_reply( reply )
	if ( reply.name == '__GOOGLE_FIGHT__' ) then
		if ( not reply.status ) then
			send_room_message( string.format( "Failed to retrieve data from '%s' ( %s ), ending fight", reply.url, reply.data ) )
			fight_terms = {}
			fight_active = false
			return
		end

		local ok, data = pcall( JSON.Decode, reply.data )
		if ( not ok ) then
			send_room_message( string.format( "Failed to parse the data from '%s', ending fight", reply.url ) )
			fight_terms = {}
			fight_active = false
			return
		end

		fight_terms[fight_idx]['count'] = tonumber( data['responseData']['cursor']['estimatedResultCount'] )

		if ( fight_idx == #fight_terms ) then
			-- fight is over, show results
			table.sort( fight_terms, function( i1, i2 ) return i1['count'] > i2['count'] end )
			local msg = "And the winner is:\n"
			for k, itm in ipairs( fight_terms ) do
				msg = msg .. string.format( "( %d ) '%s' with %d hits", k, itm['term'], itm['count'] )
				if ( k < #fight_terms ) then
					msg = msg .. "\n"
				end
			end
			send_room_message( string.format( '%s', msg ) )
			fight_terms = {}
			fight_active = false
		end

		fight_idx = fight_idx + 1
		local url = string.format( "http://ajax.googleapis.com/ajax/services/search/web?q=%s&v=1.0",
				           mbutils.url_encode( fight_terms[fight_idx]['term'] ) )
		network_request( "__GOOGLE_FIGHT__", url )

		return
	end

	if ( reply.status ) then
		local ok, data = pcall( JSON.Decode, reply.data )
		local msg = ""

		dout( "Results: " .. data['responseData']['cursor']['estimatedResultCount'] )

		if ( not ok ) then
			msg = "No results for your query"
		else
			local results = data['responseData']['results']
			local cnt = #results
			msg = "Search results:\n"
			for i = 1, cnt, 1 do
				msg = msg .. mbutils.url_decode( results[i]['titleNoFormatting'] ) .. " - " .. results[i]['url']
				if ( i < cnt ) then
					msg = msg .. "\n"
				end
			end
		end
		send_room_message( msg )
	else
		send_room_message( string.format( "Error retrieving URL '%s': %s", reply.url, reply.data ) )
	end
end

function handle_room_config( config )
end

function handle_room_message( message )
	local cmd, term_string = string.match( message.body, "(:google)%s+(.*)" )
	if ( cmd and term_string ) then
		local terms = mbutils.url_encode( term_string )
		local url = string.format( "http://ajax.googleapis.com/ajax/services/search/web?q=%s&v=1.0", terms )
		network_request( terms, url )
	end

	cmd, term_string = string.match( message.body, "(:googlefight)%s+(.*)" )
	if ( cmd and term_string ) then
		local terms = tokenize( term_string )
		if ( #terms < 2 ) then
			send_room_message( "You must specify at least 2 items for the fight" )
			return
		end
		if ( fight_active ) then
			send_room_message( "A fight is currently in progress, please wait" )
			return
		end

		fight_terms = {}
		for k, term in ipairs( terms ) do
			fight_terms[k] = {
				['term']  = term,
				['count'] = 0
			}
		end
		fight_active = true
		fight_idx = 1
		local url = string.format( "http://ajax.googleapis.com/ajax/services/search/web?q=%s&v=1.0", mbutils.url_encode( terms[1] ) )
		network_request( "__GOOGLE_FIGHT__", url )
		send_room_message( "Let's get it ON !" )
	end
end

function handle_room_presence( presence )
end

dout( "Search away !" )
