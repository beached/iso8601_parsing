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

#include <daw/daw_exception.h>
#include <daw/daw_string_view.h>

struct invalid_iso8601_timestamp {};
struct invalid_javascript_timestamp {};

namespace daw {
	struct insuffient_input {};

	namespace details {
		template<typename Result>
		constexpr Result to_integer( char const c ) noexcept {
			return static_cast<Result>( c - '0' );
		}

		template<typename Result, size_t count, typename CharT, typename Traits>
		constexpr Result consume_unsigned( daw::basic_string_view<CharT, Traits> &digit_str ) {
			static_assert( count > 0, "Must consume at least one digit from string" );
			daw::exception::precondition_check<insuffient_input>( count <= digit_str.size( ) );
			auto result = to_integer<Result>( digit_str[0] );
			for( size_t n = 1; n < count; ++n ) {
				result *= 10;
				result += to_integer<Result>( digit_str[n] );
			}
			digit_str.remove_prefix( count );
			return result;
		}

		template<typename Result, typename CharT, typename Traits>
		constexpr Result consume_unsigned( daw::basic_string_view<CharT, Traits> &digit_str, size_t const count ) {
			daw::exception::precondition_check<insuffient_input>( count <= digit_str.size( ) );
			auto result = to_integer<Result>( digit_str[0] );
			for( size_t n = 1; n < count; ++n ) {
				result *= 10;
				result += to_integer<Result>( digit_str[n] );
			}
			digit_str.remove_prefix( count );
			return result;
		}

		template<typename Result, size_t count, typename CharT>
		constexpr Result parse_unsigned( const CharT *digit_str ) noexcept {
			Result result = 0;
			for( size_t n = 0; n < count; ++n ) {
				result = static_cast<Result>( ( result << 1 ) + ( result << 3 ) ) + to_integer<Result>( digit_str[n] );
			}
			return result;
		}

		template<typename Result, typename CharT, typename Traits>
		constexpr Result parse_unsigned( daw::basic_string_view<CharT, Traits> number_string ) noexcept {
			Result result = 0;
			for( size_t n = 0; n < number_string.size( ); ++n ) {
				result = static_cast<Result>( ( result << 1 ) + ( result << 3 ) ) + to_integer<Result>( number_string[n] );
			}
			return static_cast<Result>( result );
		}

		constexpr bool is_digit( char const c ) noexcept {
			return '0' <= c && c <= '9';
		}

		constexpr bool is_digit( wchar_t const c ) noexcept {
			return L'0' <= c && c <= L'9';
		}

		template<typename CharT, typename Traits>
		constexpr bool is_digit( daw::basic_string_view<CharT, Traits> const &sv ) noexcept {
			return !sv.empty( ) && is_digit( sv.front( ) );
		}

		constexpr char to_lower( char const c ) noexcept {
			return static_cast<char>( c | ' ' );
		}

		constexpr wchar_t to_lower( wchar_t const c ) noexcept {
			return static_cast<wchar_t>( c | L' ' );
		}
	} // namespace details
} // namespace daw
