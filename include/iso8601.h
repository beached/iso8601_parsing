// The MIT License (MIT)
//
// Copyright (c) 2018 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cctype>
#include <cstdint>
#include <date/date.h>
#include <string_view>

struct invalid_iso_combinded_string {};

namespace details {
	template<typename Result, size_t count>
	constexpr Result parse_unsigned( std::string_view &digit_str ) {
		static_assert( count > 0, "Must consume at least one digit from string" );
		if( digit_str.size( ) < count ) {
			throw invalid_iso_combinded_string{};
		}
		Result result = digit_str[0] - '0';
		for( size_t n = 1; n < count; ++n ) {
			result *= 10;
			result += digit_str[n] - '0';
		}
		digit_str.remove_prefix( count );
		return result;
	}

	constexpr bool is_digit( char const c ) noexcept {
		return '0' <= c && c <= '9';
	}

	constexpr bool is_delemiter( std::string_view const &sv ) noexcept {
		return !sv.empty( ) && !is_digit( sv.front( ) );
	}

	constexpr bool is_digit( std::string_view const &sv ) noexcept {
		return !sv.empty( ) && is_digit( sv.front( ) );
	}

	constexpr char to_lower( char const c ) noexcept {
		return static_cast<unsigned char>( c ) | static_cast<unsigned char>( 0x32 );
	}

	constexpr std::chrono::minutes parse_offset( std::string_view &offset_str ) {
		using std::chrono::minutes;
		if( static_cast<uint8_t>( offset_str.empty( ) ) ) {
			return minutes{0};
		} else if( to_lower( offset_str[0] ) == 'z' ) {
			return minutes{0};
		}

		int8_t const is_negative = [&]( ) {
			switch( static_cast<uint8_t>( offset_str.front( ) ) ) {
			case '-':
				offset_str.remove_prefix( 1 );
				return -1;
			case 0xE2: // Unicode MINUS SIGN 0x2212, utf-8 0xE2,0x88, 0x92
				if( offset_str.size( ) >= 3 && static_cast<uint8_t>( offset_str[1] ) == 0x88 &&
				    static_cast<uint8_t>( offset_str[2] ) == 0x92 ) {

					offset_str.remove_prefix( 3 );
					return -1;
				}
				return 1;
			case '+':
				offset_str.remove_prefix( 1 );
				return 1;
			default:
				return 1;
			}
		}( );

		// TODO: determine if it is proper to have error condition when
		// a digit is out of range for the time unit.
		// hours
		auto offset = parse_unsigned<int16_t, 2>( offset_str ) * 60;

		if( offset_str.empty( ) ) {
			return minutes{offset * is_negative};
		} else if( !is_digit( offset_str.front( ) ) ) {
			offset_str.remove_prefix( 1 );
		}

		// minutes
		offset += parse_unsigned<int16_t, 2>( offset_str );

		return minutes{offset * is_negative};
	}

	constexpr date::year_month_day parse_iso8601_date( std::string_view &date_str ) {
		auto const y = parse_unsigned<int16_t, 4>( date_str );
		if( is_delemiter( date_str ) ) {
			date_str.remove_prefix( 1 );
		}

		auto const m = parse_unsigned<int8_t, 2>( date_str );
		if( is_delemiter( date_str ) ) {
			date_str.remove_prefix( 1 );
		}

		auto const d = parse_unsigned<int8_t, 2>( date_str );
		if( is_delemiter( date_str ) ) {
			date_str.remove_prefix( 1 );
		}
		return date::year_month_day{date::year{y}, date::month( m ), date::day( d )};
	}

	constexpr std::chrono::milliseconds parse_iso8601_time( std::string_view &time_str ) {
		auto const h = parse_unsigned<uint8_t, 2>( time_str );
		if( is_delemiter( time_str ) ) {
			time_str.remove_prefix( 1 );
		}
		auto const m = parse_unsigned<uint8_t, 2>( time_str );
		if( is_delemiter( time_str ) ) {
			time_str.remove_prefix( 1 );
		}
		auto const s = parse_unsigned<uint8_t, 2>( time_str );
		uint16_t ms = 0;
		if( time_str[0] == '.' ) {
			time_str.remove_prefix( 1 );
			if( is_digit( time_str ) ) {
				ms += 1000 * ( time_str[0] - '0' );
				time_str.remove_prefix( 1 );
				if( is_digit( time_str ) ) {
					ms += 100 * ( time_str[0] - '0' );
					time_str.remove_prefix( 1 );
					if( is_digit( time_str ) ) {
						ms += 10 * ( time_str[0] - '0' );
						time_str.remove_prefix( 1 );
					}
					if( is_digit( time_str ) ) {
						ms += time_str[0] - '0';
						time_str.remove_prefix( 1 );

						while( is_digit( time_str ) ) {
							time_str.remove_prefix( 1 );
						}
					}
				}
			}
		}
		std::chrono::milliseconds result =
		  std::chrono::hours{h} + std::chrono::minutes{m} + std::chrono::seconds{s} + std::chrono::milliseconds{ms};

		return result;
	}
} // namespace details

constexpr date::year_month_day parse_iso8601_date( std::string_view date_str ) {
	return details::parse_iso8601_date( date_str );
}

constexpr std::chrono::milliseconds parse_iso8601_time( std::string_view time_str ) {
	return details::parse_iso8601_time( time_str );
}

constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
parse_iso8601_timestamp( std::string_view timestamp_str ) {
	auto const dte = date::sys_days{details::parse_iso8601_date( timestamp_str )};
	if( details::is_delemiter( timestamp_str ) ) {
		timestamp_str.remove_prefix( 1 );
	}
	auto const tme = details::parse_iso8601_time( timestamp_str );
	auto const ofst = details::parse_offset( timestamp_str );
	auto result = ( dte + std::chrono::milliseconds( tme.count( ) ) ) - ofst;
	return result;
}

