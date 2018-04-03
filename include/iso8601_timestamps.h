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

#include "common.h"

namespace date {
	namespace details {
		constexpr bool is_delemiter( std::string_view const &sv ) noexcept {
			return !sv.empty( ) && !is_digit( sv.front( ) );
		}

		constexpr int16_t parse_offset( std::string_view &offset_str ) {
			if( static_cast<uint8_t>( offset_str.empty( ) ) || to_lower( offset_str[0] ) == 'z' ) {
				return 0;
			}

			auto const is_negative = [&]( ) -> int16_t {
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
			auto offset = consume_unsigned<int16_t, 2>( offset_str ) * 60;

			if( offset_str.empty( ) ) {
				return static_cast<int16_t>( offset * is_negative );
			} else if( !is_digit( offset_str.front( ) ) ) {
				offset_str.remove_prefix( 1 );
			}

			// minutes
			offset += consume_unsigned<int16_t, 2>( offset_str );

			return static_cast<int16_t>( offset * is_negative );
		}

		constexpr auto parse_iso8601_date( std::string_view &date_str ) {
			struct result_t {
				uint16_t y;
				uint8_t m;
				uint8_t d;
			};
			result_t result{0, 0, 0};
			result.y = consume_unsigned<uint16_t, 4>( date_str );
			if( is_delemiter( date_str ) ) {
				date_str.remove_prefix( 1 );
			}

			result.m = consume_unsigned<uint8_t, 2>( date_str );
			if( is_delemiter( date_str ) ) {
				date_str.remove_prefix( 1 );
			}

			result.d = consume_unsigned<uint8_t, 2>( date_str );
			if( is_delemiter( date_str ) ) {
				date_str.remove_prefix( 1 );
			}
			return result;
		}

		constexpr auto parse_iso8601_time( std::string_view &time_str ) {
			struct result_t {
				int8_t h;
				int8_t m;
				int8_t s;
				int16_t ms;
			};
			result_t result{0, 0, 0, 0};

			result.h = consume_unsigned<int8_t, 2>( time_str );
			if( is_delemiter( time_str ) ) {
				time_str.remove_prefix( 1 );
			}

			result.m = consume_unsigned<int8_t, 2>( time_str );
			if( is_delemiter( time_str ) ) {
				time_str.remove_prefix( 1 );
			}

			result.s = consume_unsigned<int8_t, 2>( time_str );

			if( time_str[0] == '.' ) {
				time_str.remove_prefix( 1 );
				if( is_digit( time_str ) ) {
					if( is_digit( time_str ) ) {
						result.ms += 100 * to_integer<int16_t>( time_str[0] );
						time_str.remove_prefix( 1 );
						if( is_digit( time_str ) ) {
							result.ms += 10 * to_integer<int16_t>( time_str[0] );
							time_str.remove_prefix( 1 );
						}
						if( is_digit( time_str ) ) {
							result.ms += to_integer<int16_t>( time_str[0] );
							time_str.remove_prefix( 1 );

							while( is_digit( time_str ) ) {
								time_str.remove_prefix( 1 );
							}
						}
					}
				}
			}
			return result;
		}
	} // namespace details

	constexpr date::year_month_day parse_iso8601_date( std::string_view date_str ) {
		auto const tmp = details::parse_iso8601_date( date_str );
		return date::year_month_day{date::year{tmp.y}, date::month( tmp.m ), date::day( tmp.d )};
	}

	constexpr std::chrono::milliseconds parse_iso8601_time( std::string_view time_str ) {
		auto const tmp = details::parse_iso8601_time( time_str );
		std::chrono::milliseconds result = std::chrono::hours{tmp.h} + std::chrono::minutes{tmp.m} +
		                                   std::chrono::seconds{tmp.s} + std::chrono::milliseconds{tmp.ms};

		return result;
	}

	constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
	parse_iso8601_timestamp( std::string_view timestamp_str ) {
		auto const dte = details::parse_iso8601_date( timestamp_str );
		if( details::is_delemiter( timestamp_str ) ) {
			timestamp_str.remove_prefix( 1 );
		}
		auto const tme = details::parse_iso8601_time( timestamp_str );
		auto const ofst = details::parse_offset( timestamp_str );
		std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> result{
		  date::sys_days{date::year{dte.y} / date::month( dte.m ) / date::day( dte.d )} + std::chrono::hours{tme.h} +
		  std::chrono::minutes{tme.m} + std::chrono::seconds{tme.s} + std::chrono::milliseconds{tme.ms}};

		result = result - std::chrono::minutes{ofst};
		return result;
	}

	constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
	parse_javascript_timestamp( std::string_view timestamp_str ) {
		if( timestamp_str.size( ) != 24 || details::to_lower( timestamp_str[23] ) != 'z' ) {
			throw invalid_javascript_timestamp{};
		}
		auto const yr = details::parse_unsigned<uint16_t, 4>( timestamp_str.data( ) );
		auto const mo = details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 5 );
		auto const dy = details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 8 );
		auto const hr = details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 11 );
		auto const mi = details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 14 );
		auto const sc = details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 17 );
		auto const ms = details::parse_unsigned<uint16_t, 3>( timestamp_str.data( ) + 20 );

		std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> result{
		  date::sys_days{date::year{yr} / date::month( mo ) / date::day( dy )} + std::chrono::hours{hr} +
		  std::chrono::minutes{mi} + std::chrono::seconds{sc} + std::chrono::milliseconds{ms}};

		return result;
	}
} // namespace date
