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

#include <daw/daw_string_view.h>

namespace date {
	struct invalid_iso8601_timestamp {};
	struct invalid_javascript_timestamp {};
	struct insuffient_input {};

	namespace details {
		template<typename Result>
		constexpr Result to_integer( char const c ) noexcept {
			return static_cast<Result>( c - '0' );
		}

		template<typename Result, size_t count>
		constexpr Result consume_unsigned( std::string_view &digit_str ) {
			static_assert( count > 0, "Must consume at least one digit from string" );
			if( digit_str.size( ) < count ) {
				throw insuffient_input{};
			}
			auto result = to_integer<Result>( digit_str[0] );
			for( size_t n = 1; n < count; ++n ) {
				result *= 10;
				result += to_integer<Result>( digit_str[n] );
			}
			digit_str.remove_prefix( count );
			return result;
		}

		template<typename Result>
		constexpr Result consume_unsigned( std::string_view &digit_str, size_t const count ) {
			if( digit_str.size( ) < count ) {
				throw insuffient_input{};
			}
			auto result = to_integer<Result>( digit_str[0] );
			for( size_t n = 1; n < count; ++n ) {
				result *= 10;
				result += to_integer<Result>( digit_str[n] );
			}
			digit_str.remove_prefix( count );
			return result;
		}

		template<typename Result, size_t count>
		constexpr Result parse_unsigned( char const *digit_str ) {
			static_assert( count > 0, "Must consume at least one digit from string" );
			auto result = to_integer<Result>( digit_str[0] );
			for( size_t n = 1; n < count; ++n ) {
				result *= 10;
				result += to_integer<Result>( digit_str[n] );
			}
			return result;
		}

		template<typename Result, typename CharT, typename Traits>
		constexpr Result parse_unsigned( daw::basic_string_view<CharT, Traits> number_string ) noexcept {
			auto result = 0;
			for( size_t n = 0; n < number_string.size( ); ++n ) {
				result *= 10;
				result += to_integer<Result>( number_string[n] );
			}
			return result;
		}

		constexpr bool is_digit( char const c ) noexcept {
			return '0' <= c && c <= '9';
		}

		constexpr bool is_digit( std::string_view const &sv ) noexcept {
			return !sv.empty( ) && is_digit( sv.front( ) );
		}

		constexpr char to_lower( char const c ) noexcept {
			return static_cast<char>( static_cast<unsigned char>( c ) | static_cast<unsigned char>( 0x32u ) );
		}
	} // namespace details
} // namespace date
