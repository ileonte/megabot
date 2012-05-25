local type = type
local next = next
local pairs = pairs
local tostring = tostring
local tonumber = tonumber
local string = require( 'string' )
local debug = require( 'debug' )

module( "mbutils" )

function show_table( t, name, indent )
	local cart
	local autoref

	local function isemptytable( t )
		return next( t ) == nil
	end

	local function basicSerialize( o )
		local so = tostring( o )
		if type( o ) == "function" then
			local info = debug.getinfo( o, "S" )
			if info.what == "C" then
				return string.format( "%q", so .. ", C function" )
			else
				return string.format( "%q", so .. ", defined in (" ..
						info.linedefined .. "-" .. info.lastlinedefined ..
						")" .. info.source )
			end
		elseif type( o ) == "number" then
			return so
		else
			return string.format( "%q", so )
		end
	end

	local function addtocart( value, name, indent, saved, field )
		indent = indent or ""
		saved = saved or {}
		field = field or name

		cart = cart .. indent .. field

		if type( value ) ~= "table" then
			cart = cart .. " = " .. basicSerialize( value ) .. ";\n"
		else
			if saved[value] then
				cart = cart .. " = {}; -- " .. saved[value] .. " (self reference)\n"
				autoref = autoref ..  name .. " = " .. saved[value] .. ";\n"
			else
				saved[value] = name
				if isemptytable( value ) then
					cart = cart .. " = {};\n"
				else
					cart = cart .. " = {\n"
					for k, v in pairs( value ) do
						k = basicSerialize( k )
						local fname = string.format( "%s[%s]", name, k )
						field = string.format( "[%s]", k )
						addtocart( v, fname, indent .. "   ", saved, field )
					end
					cart = cart .. indent .. "};\n"
				end
			end
		end
	end

	name = name or "__unnamed__"
	if type( t ) ~= "table" then
		return name .. " = " .. basicSerialize( t )
	end
	cart, autoref = "", ""
	addtocart( t, name, indent )
	return cart .. autoref
end

function url_encode(str)
	if ( str ) then
		str = string.gsub( str, "\n", "\r\n" )
		str = string.gsub( str, "([^%w ])", function( c ) return string.format( "%%%02X", string.byte( c ) ) end )
		str = string.gsub( str, " ", "+" )
	end
	return str
end

function url_decode(str)
	str = string.gsub( str, "+", " " )
	str = string.gsub( str, "%%(%x%x)", function( h ) return string.char( tonumber( h, 16 ) ) end )
	str = string.gsub( str, "&#(%d%d);", function( h ) return string.char( tonumber( h, 10 ) ) end )
	str = string.gsub( str, "\r\n", "\n" )
	return str
end

function trim( str )
        if ( str == nil ) then
                return ''
        end
        return string.gsub( string.gsub( tostring( str ), "[%s]+$", "" ), "^[%s]+", "" )
end

