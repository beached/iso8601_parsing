// The MIT License (MIT)
//
// Copyright (c) 2018-2019 Darrell Wright
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
#include <daw/daw_string_view.h>

#include "common.h"

namespace date_parsing {
	namespace details {
		constexpr bool is_delemiter( daw::string_view const &sv ) noexcept {
			return !sv.empty( ) && !daw::details::is_digit( sv.front( ) );
		}

		constexpr int16_t parse_offset( daw::string_view &offset_str ) {
			if( static_cast<uint8_t>( offset_str.empty( ) ) || daw::details::to_lower( offset_str[0] ) == 'z' ) {
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
			auto offset = daw::details::consume_unsigned<int16_t, 2>( offset_str ) * 60;

			if( offset_str.empty( ) ) {
				return static_cast<int16_t>( offset * is_negative );
			} else if( !daw::details::is_digit( offset_str.front( ) ) ) {
				offset_str.remove_prefix( 1 );
			}

			// minutes
			offset += daw::details::consume_unsigned<int16_t, 2>( offset_str );

			return static_cast<int16_t>( offset * is_negative );
		}

		constexpr auto parse_iso8601_date( daw::string_view &date_str ) noexcept {
			struct result_t {
				uint16_t y;
				uint8_t m;
				uint8_t d;
			};
			result_t result{0, 0, 0};
			result.y = daw::details::consume_unsigned<uint16_t, 4>( date_str );
			if( is_delemiter( date_str ) ) {
				date_str.remove_prefix( 1 );
			}

			result.m = daw::details::consume_unsigned<uint8_t, 2>( date_str );
			if( is_delemiter( date_str ) ) {
				date_str.remove_prefix( 1 );
			}

			result.d = daw::details::consume_unsigned<uint8_t, 2>( date_str );
			if( is_delemiter( date_str ) ) {
				date_str.remove_prefix( 1 );
			}
			return result;
		}

		constexpr auto parse_iso8601_time( daw::string_view &time_str ) noexcept {
			struct result_t {
				int8_t h;
				int8_t m;
				int8_t s;
				int16_t ms;
			};
			result_t result{0, 0, 0, 0};

			result.h = daw::details::consume_unsigned<int8_t, 2>( time_str );
			if( is_delemiter( time_str ) ) {
				time_str.remove_prefix( 1 );
			}

			result.m = daw::details::consume_unsigned<int8_t, 2>( time_str );
			if( is_delemiter( time_str ) ) {
				time_str.remove_prefix( 1 );
			}

			result.s = daw::details::consume_unsigned<int8_t, 2>( time_str );

			if( time_str[0] == '.' ) {
				time_str.remove_prefix( 1 );
				if( daw::details::is_digit( time_str ) ) {
					if( daw::details::is_digit( time_str ) ) {
						result.ms += 100 * daw::details::to_integer<int16_t>( time_str[0] );
						time_str.remove_prefix( 1 );
						if( daw::details::is_digit( time_str ) ) {
							result.ms += 10 * daw::details::to_integer<int16_t>( time_str[0] );
							time_str.remove_prefix( 1 );
						}
						if( daw::details::is_digit( time_str ) ) {
							result.ms += daw::details::to_integer<int16_t>( time_str[0] );
							time_str.remove_prefix( 1 );

							while( daw::details::is_digit( time_str ) ) {
								time_str.remove_prefix( 1 );
							}
						}
					}
				}
			}
			return result;
		}
	} // namespace details

	constexpr date::year_month_day parse_iso8601_date( daw::string_view date_str ) {
		auto const tmp = details::parse_iso8601_date( date_str );
		return date::year_month_day{date::year{tmp.y}, date::month( tmp.m ), date::day( tmp.d )};
	}

	constexpr std::chrono::milliseconds parse_iso8601_time( daw::string_view time_str ) {
		auto const tmp = details::parse_iso8601_time( time_str );
		std::chrono::milliseconds result = std::chrono::hours{tmp.h} + std::chrono::minutes{tmp.m} +
		                                   std::chrono::seconds{tmp.s} + std::chrono::milliseconds{tmp.ms};

		return result;
	}

	constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
	parse_iso8601_timestamp( daw::string_view timestamp_str ) {
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
	template<typename CharT, typename Traits>
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
	parse_iso8601_timestamp( std::basic_string<CharT, Traits> const &timestamp_str ) {
		return parse_iso8601_timestamp( daw::make_string_view( timestamp_str ) );
	}

	template<typename CharT, size_t N>
	constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
	parse_iso8601_timestamp( CharT const ( &timestamp_str )[N] ) {
		return parse_iso8601_timestamp( daw::basic_string_view<CharT>{timestamp_str, N - 1} );
	}

	template<typename CharT, typename Traits>
	constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
	parse_javascript_timestamp( daw::basic_string_view<CharT, Traits> timestamp_str ) {
		if( timestamp_str.size( ) != 24 || daw::details::to_lower( timestamp_str[23] ) != 'z' ) {
			throw invalid_javascript_timestamp{};
		}
		auto const yr = daw::details::parse_unsigned<uint16_t, 4>( timestamp_str.data( ) );
		auto const mo = daw::details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 5 );
		auto const dy = daw::details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 8 );
		auto const hr = daw::details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 11 );
		auto const mi = daw::details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 14 );
		auto const sc = daw::details::parse_unsigned<uint8_t, 2>( timestamp_str.data( ) + 17 );
		auto const ms = daw::details::parse_unsigned<uint16_t, 3>( timestamp_str.data( ) + 20 );

		std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> result{
		  date::sys_days{date::year{yr} / date::month( mo ) / date::day( dy )} + std::chrono::hours{hr} +
		  std::chrono::minutes{mi} + std::chrono::seconds{sc} + std::chrono::milliseconds{ms}};

		return result;
	}

	template<typename CharT, typename Traits>
	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
	parse_javascript_timestamp( std::basic_string<CharT, Traits> const &timestamp_str ) {
		return parse_javascript_timestamp( daw::make_string_view( timestamp_str ) );
	}

	template<typename CharT, size_t N>
	constexpr std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
	parse_javascript_timestamp( CharT const ( &timestamp_str )[N] ) {
		return parse_javascript_timestamp( daw::basic_string_view<CharT>{timestamp_str, N - 1} );
	}
} // namespace date_parsing
