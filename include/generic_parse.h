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

#include <chrono>
#include <cstdint>
#include <date/date.h>
#include <daw/daw_string_view.h>

#include "common.h"

namespace date {
	struct invalid_date_field {};
	struct unsupported_date_field {};
	namespace impl {
		template<typename T, typename U>
		constexpr void default_width( T &value, U def_value ) noexcept {
			if( value < 1 ) {
				value = static_cast<T>( def_value );
			}
		}
	} // namespace impl

	namespace formats {
		namespace impl {
			template<typename OutputIterator>
			constexpr void output_digit( char, OutputIterator &oi, uint8_t digit ) {
				*oi = '0' + digit;
				++oi;
			}

			template<typename OutputIterator>
			constexpr void output_digit( wchar_t, OutputIterator &oi, uint8_t digit ) {
				*oi = L'0' + digit;
				++oi;
			}

			template<typename Result>
			constexpr Result pow10( size_t exponent ) noexcept {
				Result result = 1;
				while( exponent > 1 ) {
					result *= 10;
					--exponent;
				}
				return result;
			}

			template<typename Result>
			constexpr Result log10( size_t n ) noexcept {
				Result result = 0;
				while( n >= 10 ) {
					++result;
					n /= 10;
				}
				return result;
			}

			template<typename CharT, typename OutputIterator, typename Unsigned>
			constexpr void output_digits( CharT, size_t width, OutputIterator &oi, Unsigned digits ) {
				Unsigned divisor = pow10<Unsigned>( width + 1 );
				while( divisor > 1 ) {
					divisor /= 10;
					auto const digit = digits / divisor;
					output_digit( CharT{}, oi, static_cast<uint8_t>( digit ) );
					digits -= digit * divisor;
				}
			}

			constexpr int format_width( int width, int &value ) noexcept {
				--width; // Using powers of 10
				auto places = log10<int>( value );
				if( width < 0 ) {
					width = places;
				} else if( width < places ) {
					while( width < places ) {
						auto const pw = impl::pow10<int>( places + 1 );
						auto const digit = value / pw;
						value -= digit * pw;
						--places;
					}
				}
				++width; // Back to width in character count
				return width;
			}
		} // namespace impl

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Year {
			int field_width;

			constexpr Year( ) noexcept
			  : field_width{-1} {}

			constexpr Year( int w ) noexcept
			  : field_width{w} {}

			constexpr Year( Year const &rhs ) noexcept
			  : field_width{rhs.field_width} {}

			constexpr Year( Year &&rhs ) noexcept
			  : field_width{rhs.field_width} {}

			constexpr Year &operator=( Year const &other ) noexcept {
				field_width = other.field_width;
				return *this;
			}

			constexpr Year &operator=( Year &&other ) noexcept {
				field_width = std::move( other.field_width );
				return *this;
			}

			~Year( ) noexcept = default;

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto yr = static_cast<int>( date::year_month_day{date::floor<date::days>( tp )}.year( ) );
				auto width = impl::format_width( field_width, yr );
				impl::output_digits( CharT{}, width, oi, yr );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Month {
			int field_width;

			constexpr Month( ) noexcept
			  : field_width{-1} {}

			constexpr Month( int w ) noexcept
			  : field_width{w} {}

			constexpr Month( Month const &rhs ) noexcept
			  : field_width{rhs.field_width} {}

			constexpr Month( Month &&rhs ) noexcept
			  : field_width{rhs.field_width} {}

			constexpr Month &operator=( Month const &other ) noexcept {
				field_width = other.field_width;
				return *this;
			}

			constexpr Month &operator=( Month &&other ) noexcept {
				field_width = std::move( other.field_width );
				return *this;
			}

			~Month( ) noexcept = default;

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto mo =
				  static_cast<int>( static_cast<unsigned>( date::year_month_day{date::floor<date::days>( tp )}.month( ) ) );
				auto width = impl::format_width( field_width, mo );
				impl::output_digits( CharT{}, width, oi, mo );
			}
		};

		struct Month_Name;

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Day {
			int field_width;

			constexpr Day( ) noexcept
			  : field_width{-1} {}

			constexpr Day( int w ) noexcept
			  : field_width{w} {}

			constexpr Day( Day const &rhs ) noexcept
			  : field_width{rhs.field_width} {}

			constexpr Day( Day &&rhs ) noexcept
			  : field_width{rhs.field_width} {}

			constexpr Day &operator=( Day const &other ) noexcept {
				field_width = other.field_width;
				return *this;
			}

			constexpr Day &operator=( Day &&other ) noexcept {
				field_width = std::move( other.field_width );
				return *this;
			}

			~Day( ) noexcept = default;

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto dy =
				  static_cast<int>( static_cast<unsigned>( date::year_month_day{date::floor<date::days>( tp )}.day( ) ) );
				auto width = impl::format_width( field_width, dy );
				impl::output_digits( CharT{}, field_width, oi, dy );
			}
		};

		struct Day_of_Week;

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Day_of_Year {
			int field_width = -1;

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto const ymd = year_month_day{date::floor<date::days>( tp )};
				auto const jan1 = year_month_day{ymd.year( ) / 1 / 1};
				auto diff = floor<days>( static_cast<sys_days>( ymd ) - static_cast<sys_days>( jan1 ) ).count( ) + 1;
				auto width = impl::format_width( field_width, diff );
				impl::output_digits( CharT{}, width, oi, diff );
			}
		};

		enum class hour_formats { twelve_hour, twenty_four_hour };

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Hour {
			int field_width = -1;
			hour_formats hour_format = hour_formats::twenty_four_hour;

			constexpr Hour( ) noexcept {}

			constexpr Hour( int w ) noexcept
			  : field_width{w} {}

			constexpr Hour( int w, hour_formats format ) noexcept
			  : field_width{w}
			  , hour_format{format} {}

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto const dte = date::floor<date::days>( tp );
				auto const tod = date::make_time( tp - dte );
				auto hr = static_cast<int>( tod.hours( ).count( ) );
				if( hour_format == hour_formats::twelve_hour && hr >= 12 ) {
					hr -= 12;
				}
				auto width = impl::format_width( field_width, hr );
				impl::output_digits( CharT{}, width, oi, hr );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Minute {
			int field_width = -1;

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto const dte = date::floor<date::days>( tp );
				auto const tod = date::make_time( tp - dte );
				auto hr = static_cast<int>( tod.minutes( ).count( ) );
				auto width = impl::format_width( field_width, hr );
				impl::output_digits( CharT{}, width, oi, hr );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Second {
			int field_width = -1;

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto const dte = date::floor<date::days>( tp );
				auto const tod = date::make_time( tp - dte );
				auto hr = static_cast<int>( tod.seconds( ).count( ) );
				auto width = impl::format_width( field_width, hr );
				impl::output_digits( CharT{}, width, oi, hr );
			}
		};

		namespace impl {
			constexpr char default_separator( char ) noexcept {
				return '-';
			}
			constexpr wchar_t default_separator( wchar_t ) noexcept {
				return L'-';
			}
		} // namespace impl

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct YearMonthDay {
			CharT separator = impl::default_separator( CharT{} );

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				Year<CharT, Traits>{}( tp, oi );
				*oi = separator;
				++oi;
				Month<CharT, Traits>{}( tp, oi );
				*oi = separator;
				++oi;
				Day<CharT, Traits>{}( tp, oi );
			}
		};

		// Get nth value from a parameter pack
		namespace impl {
			template<size_t index, typename CharT, typename Traits, typename Duration, typename OutputIterator>
			constexpr void get_string_value( size_t const, date::sys_time<Duration> const &, OutputIterator & ) noexcept {}

			template<size_t index, typename CharT, typename Traits, typename Duration, typename OutputIterator, typename Arg,
			         typename... Args>
			constexpr void get_string_value( size_t const n, date::sys_time<Duration> const &tp, OutputIterator &oi,
			                                 Arg &&arg, Args &&... args ) {
				if( index == n ) {
					arg( tp, oi );
				}
				return get_string_value<index + 1, CharT, Traits>( n, tp, oi, std::forward<Args>( args )... );
			}
		} // namespace impl

		template<typename CharT, typename Traits, typename Duration, typename OutputIterator, typename... Args>
		constexpr void get_string_value( size_t const n, date::sys_time<Duration> const &tp, OutputIterator &oi,
		                                 Args &&... args ) {
			if( n >= sizeof...( Args ) ) {
				throw std::out_of_range{"Invalid index to parameter pack"};
			}
			impl::get_string_value<0, CharT, Traits>( n, tp, oi, std::forward<Args>( args )... );
		}

	} // namespace formats

	namespace impl {
		template<typename ForwardIterator, typename ForwardIteratorLast, typename OutputIterator>
		constexpr OutputIterator copy( ForwardIterator first, ForwardIteratorLast last, OutputIterator oi ) {
			while( first != last ) {
				*oi = *first;
				++oi;
				++first;
			}
			return oi;
		}
	} // namespace impl
	template<typename CharT, typename Traits, typename OutputIterator, typename Duration, typename... FormatFlags>
	constexpr auto fmt( daw::basic_string_view<CharT, Traits> fmt_str, date::sys_time<Duration> const &tp,
	                    OutputIterator &oi, FormatFlags &&... flags )
	  -> std::enable_if_t<( sizeof...( FormatFlags ) > 0 ), OutputIterator> {

		auto pos_first = fmt_str.find_first_of( static_cast<CharT>( '{' ) );
		while( pos_first != fmt_str.npos ) {
			if( pos_first != 0 ) {
				oi = impl::copy( fmt_str.cbegin( ), fmt_str.cbegin( ) + pos_first, oi );
				fmt_str.remove_prefix( pos_first );
			}
			// TODO: deal with escaped { e.g /{

			auto const pos_last = fmt_str.find_first_of( static_cast<CharT>( '}' ) );
			if( pos_last == fmt_str.npos || ( pos_last ) < 2 ) {
				throw invalid_date_field{};
			}
			auto const idx = details::parse_unsigned<size_t>( fmt_str.substr( 1, pos_last - 1 ) );
			formats::get_string_value<CharT, Traits>( idx, tp, oi, std::forward<FormatFlags>( flags )... );
			fmt_str.remove_prefix( pos_last );
			if( !fmt_str.empty( ) ) {
				fmt_str.remove_prefix( 1 );
			}
			pos_first = fmt_str.find_first_of( static_cast<CharT>( '{' ) );
		}
		if( !fmt_str.empty( ) ) {
			oi = impl::copy( fmt_str.cbegin( ), fmt_str.cend( ), oi );
		}
		return oi;
	}

	// TODO: split into char/wchar_t versions
	template<typename CharT, typename Traits, typename OutputIterator, typename Duration>
	constexpr OutputIterator fmt( daw::basic_string_view<CharT, Traits> fmt_str, date::sys_time<Duration> const &tp,
	                              OutputIterator &oi ) {

		auto pos_first = fmt_str.find_first_of( '%' );
		while( !fmt_str.empty( ) && pos_first != fmt_str.npos ) {
			if( pos_first != 0 ) {
				oi = impl::copy( fmt_str.cbegin( ), fmt_str.cbegin( ) + pos_first, oi );
				fmt_str.remove_prefix( pos_first );
			}
			if( fmt_str.empty( ) ) {
				break;
			}
			fmt_str.remove_prefix( 1 );
			int current_width = -1;
			if( details::is_digit( fmt_str.front( ) ) ) {
				current_width = details::to_integer<int>( fmt_str.front( ) );
				fmt_str.remove_prefix( 1 );
				while( details::is_digit( fmt_str.front( ) ) ) {
					current_width *= 10;
					current_width = details::to_integer<int>( fmt_str.front( ) );
					fmt_str.remove_prefix( 1 );
				}
			}
			switch( fmt_str.front( ) ) {
			case 'a':
			case 'A':
			case 'b':
			case 'B':
			case 'c':
				throw unsupported_date_field{};
			case 'C':
				impl::default_width( current_width, 2 );
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Year<CharT, Traits>{current_width} );
				break;
			case 'd':
			case 'e':
				impl::default_width( current_width, 2 );
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Month<CharT, Traits>{current_width} );
				break;
			case 'D':
				impl::default_width( current_width, 2 );
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Year<CharT, Traits>{current_width} );
				*oi = '/';
				++oi;
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Month<CharT, Traits>{current_width} );
				*oi = '/';
				++oi;
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Day<CharT, Traits>{current_width} );
				break;
			case 'F':
				impl::default_width( current_width, 4 );
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Year<CharT, Traits>{current_width} );
				*oi = '-';
				++oi;
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Month<CharT, Traits>{2} );
				*oi = '-';
				++oi;
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Day<CharT, Traits>{2} );
				break;
			case 'g':
			case 'G':
			case 'h':
				throw unsupported_date_field{};
			case 'H':
				impl::default_width( current_width, 2 );
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Hour<CharT, Traits>{current_width} );
				break;
			case 'I':
				impl::default_width( current_width, 2 );
				formats::get_string_value<CharT, Traits>(
				  0, tp, oi, formats::Hour<CharT, Traits>{current_width, formats::hour_formats::twelve_hour} );
				break;
			case 'j':
				impl::default_width( current_width, 3 );
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Day_of_Year<CharT, Traits>{current_width} );
				break;
			case 'm':
				impl::default_width( current_width, 2 );
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Month<CharT, Traits>{current_width} );
				break;
			case 'M':
				impl::default_width( current_width, 2 );
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Minute<CharT, Traits>{current_width} );
				break;
			case 'Y':
				impl::default_width( current_width, 4 );
				formats::get_string_value<CharT, Traits>( 0, tp, oi, formats::Year<CharT, Traits>{current_width} );
				break;
			default:
				throw invalid_date_field{};
			}
			fmt_str.remove_prefix( 1 );
			pos_first = fmt_str.find_first_of( '%' );
		}
		if( !fmt_str.empty( ) ) {
			oi = impl::copy( fmt_str.cbegin( ), fmt_str.cend( ), oi );
		}
		return oi;
	}

	template<typename CharT, size_t N, typename OutputIterator, typename Duration, typename... FormatFlags>
	constexpr OutputIterator fmt( CharT const ( &fmt_str )[N], date::sys_time<Duration> const &tp, OutputIterator oi,
	                              FormatFlags &&... flags ) {

		return fmt( daw::basic_string_view<CharT>{fmt_str, N}, tp, oi, flags... );
	}

} // namespace date
