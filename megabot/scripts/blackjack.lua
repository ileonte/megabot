require( 'mbutils' )

state = {}

function card_name( value )
	local cards = { 'A', '2', '3', '4', '5', '6', '7', '8', '9', '10', 'A', 'J', 'Q', 'K' }
	if ( value >= 1 and value <= #cards ) then
		return cards[value]
	end
	return 'WTF'
end

function handle_room_config( config )
end

function handle_room_message( message )
	if ( message.body == ":hitme" ) then
		if ( type( state[message.nickName] ) ~= "table" ) then
			state[message.nickName] = {}
		end

		local card = math.random( 2, 14 )
		local sum = 0
		local cards = ''
		for k, value in ipairs( state[message.nickName] ) do
			sum = sum + value
		end
		if ( sum == 0 ) then
			table.insert( state[message.nickName], card )
			sum = sum + card
		else
			if ( sum + card > 21 ) then
				if ( card == 11 ) then
					card = 1
				end
				sum = 0
				for k, value in ipairs( state[message.nickName] ) do
					if ( value == 11 ) then
						value = 1
						state[message.nickName][k] = 1
					end
					sum = sum + value
				end
			end
			sum = sum + card
			table.insert( state[message.nickName], card )
		end

		for k, value in ipairs( state[message.nickName] ) do
			cards = cards .. card_name( value ) .. " "
		end
		if ( sum < 21 ) then
			send_room_message( string.format( "%s: %s- sum = %d", message.nickName, cards, sum ) )
		elseif ( sum > 21 ) then
			send_room_message( string.format( "%s: You LOST ! %s- sum = %d", message.nickName, cards, sum ) )
			state[message.nickName] = {}
		else
			send_room_message( string.format( "%s: You WON ! %s- sum = %d", message.nickName, cards, sum ) )
			state[message.nickName] = {}
		end
	end
end

function handle_room_presence( presence )
	if ( presence.presence == PRES_Available or state.presence == PRES_Unavailable ) then
		state[presence.nickName] = {}
	end
end

math.randomseed( os.time() )
dout( "Blackjack running" )
