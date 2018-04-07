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

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iterator>
#include <string>
#include <vector>

#include <date/date.h>
#include <daw/cpp_17.h>
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

		template<typename OutputIterator, typename CharT>
		constexpr void put_char( OutputIterator &oi, CharT c ) {
			*oi++ = c;
		}
	} // namespace impl

	namespace formats {
		namespace impl {
			template<typename OutputIterator>
			constexpr void output_digit( char, OutputIterator &oi, uint8_t digit ) {
				date::impl::put_char( oi, '0' + digit );
			}

			template<typename OutputIterator>
			constexpr void output_digit( wchar_t, OutputIterator &oi, uint8_t digit ) {
				date::impl::put_char( oi, L'0' + digit );
			}

			template<typename Result, typename T>
			constexpr Result pow10( T exponent ) noexcept {
				Result result = 1;
				while( exponent > 1 ) {
					result *= 10;
					--exponent;
				}
				return result;
			}

			template<typename Result, typename T>
			constexpr Result log10( T n ) noexcept {
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

			template<typename Duration, typename OutputIterator>
			void localize( char, date::sys_time<Duration> const &tp, OutputIterator &oi, daw::string_view fmt ) {
				auto tmp = std::chrono::system_clock::to_time_t( tp );
				// Using string because of SSO and that explains the 15 char size.
				// The 30 increases is a WAG
				std::string buff( static_cast<size_t>(15), '\0' );
				auto result = std::strftime( buff.data( ), buff.size( ), fmt.data( ), std::localtime( &tmp ) );
				while( result == 0 ) {
					buff.resize( buff.size( ) + 30 );
					result = std::strftime( buff.data( ), buff.size( ), fmt.data( ), std::localtime( &tmp ) );
				}
				oi = std::copy( buff.data( ), buff.data( ) + result, oi );
			}

			template<typename Duration, typename OutputIterator>
			void localize( wchar_t, date::sys_time<Duration> const &tp, OutputIterator &oi, daw::wstring_view fmt ) {
				auto tmp = std::chrono::system_clock::to_time_t( tp );
				// Using string because of SSO and that explains the 7 wchar_t size.
				// The 30 increases is a WAG
				std::wstring buff( static_cast<size_t>(7), L'\0' );
				auto result = std::wcsftime( buff.data( ), buff.size( ), fmt.data( ), std::localtime( &tmp ) );
				while( result == 0 ) {
					buff.resize( buff.size( ) + 30 );
					result = std::wcsftime( buff.data( ), buff.size( ), fmt.data( ), std::localtime( &tmp ) );
				}
				oi = std::copy( buff.data( ), buff.data( ) + result, oi );
			}

			enum class locale_name_formats { abbreviated, full, none, alternate };

		} // namespace impl

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Century {
			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto yr = static_cast<int>( date::year_month_day{date::floor<date::days>( tp )}.year( ) );
				yr /= 100;
				impl::output_digits( CharT{}, 2, oi, yr );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Year {
			int field_width = -1;
			impl::locale_name_formats locale_name_format = impl::locale_name_formats::none;

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				if( locale_name_format == impl::locale_name_formats::alternate ) {
					formats::impl::localize( CharT{}, tp, oi, "%EY" );
				} else {
					auto yr = static_cast<int>( date::year_month_day{ date::floor<date::days>( tp ) }.year( ) );
					auto width = impl::format_width( field_width, yr );
					impl::output_digits( CharT{ }, width, oi, yr );
				}
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Month {
			int field_width = -1;

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto mo =
				  static_cast<int>( static_cast<unsigned>( date::year_month_day{date::floor<date::days>( tp )}.month( ) ) );
				if( field_width == 0 ) {
					impl::output_digits( CharT{}, 2, oi, mo - 1 );
				} else {
					impl::output_digits( CharT{}, 2, oi, mo );
				}
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Day {
			int field_width = -1;

			template<typename Duration, typename OutputIterator>
			constexpr void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				auto dy =
				  static_cast<int>( static_cast<unsigned>( date::year_month_day{date::floor<date::days>( tp )}.day( ) ) );
				auto width = impl::format_width( field_width, dy );
				impl::output_digits( CharT{}, field_width, oi, dy );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Day_of_Week {
			impl::locale_name_formats locale_name_format = impl::locale_name_formats::full;

			template<typename Duration, typename OutputIterator>
			void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				if( locale_name_format == impl::locale_name_formats::full ) {
					impl::localize( CharT{}, tp, oi, "%A" );
				} else {
					impl::localize( CharT{}, tp, oi, "%a" );
				}
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct MonthName {
			impl::locale_name_formats locale_name_format = impl::locale_name_formats::full;

			template<typename Duration, typename OutputIterator>
			void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				if( locale_name_format == impl::locale_name_formats::full ) {
					impl::localize( CharT{}, tp, oi, "%B" );
				} else {
					impl::localize( CharT{}, tp, oi, "%b" );
				}
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct LocaleDateTime {
			template<typename Duration, typename OutputIterator>
			void operator( )( date::sys_time<Duration> const &tp, OutputIterator &oi ) const {
				impl::localize( CharT{}, tp, oi, "%c" );
			}
		};

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
				date::impl::put_char( oi, separator );
				Month<CharT, Traits>{}( tp, oi );
				date::impl::put_char( oi, separator );
				Day<CharT, Traits>{}( tp, oi );
			}
		};

		// Get nth value from a parameter pack
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

			template<typename CharT, typename Traits, typename Duration, typename OutputIterator, typename Arg>
			constexpr auto runarg( date::sys_time<Duration> const &tp, OutputIterator &oi, Arg &&arg )
			  -> std::enable_if_t<daw::is_callable_v<Arg, date::sys_time<Duration> const &, OutputIterator>> {

				arg( tp, oi );
			}

			template<typename CharT, typename Traits, typename Duration, typename OutputIterator, typename Arg>
			auto runarg( date::sys_time<Duration> const &tp, OutputIterator &oi, Arg &&arg )
			  -> std::enable_if_t<!daw::is_callable_v<Arg, date::sys_time<Duration> const &, OutputIterator>> {

				std::basic_string<CharT, Traits> result = arg( );
				oi = std::copy( result.cbegin( ), result.cend( ), oi );
			}

			template<size_t index, typename CharT, typename Traits, typename Duration, typename OutputIterator>
			constexpr void get_string_value( size_t const, date::sys_time<Duration> const &, OutputIterator & ) noexcept {}

			template<size_t index, typename CharT, typename Traits, typename Duration, typename OutputIterator, typename Arg,
			         typename... Args>
			constexpr void get_string_value( size_t const n, date::sys_time<Duration> const &tp, OutputIterator &oi,
			                                 Arg &&arg, Args &&... args ) {
				if( index == n ) {
					runarg<CharT, Traits>( tp, oi, std::forward<Arg>( arg ) );
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
		template<typename OutputIterator>
		constexpr void put_newline( char, OutputIterator &oi ) {
			put_char( oi, '\n' );
		}

		template<typename OutputIterator>
		constexpr void put_newline( wchar_t, OutputIterator &oi ) {
			put_char( oi, L'\n' );
		}

			template<typename OutputIterator>
		constexpr void put_percent( char, OutputIterator &oi ) {
			put_char( oi, '%' );
		}

		template<typename OutputIterator>
		constexpr void put_percent( wchar_t, OutputIterator &oi ) {
			put_char( oi, L'%' );
		}

		template<typename OutputIterator>
		constexpr void put_tab( char, OutputIterator &oi ) {
			put_char( oi, '\t' );
		}

		template<typename OutputIterator>
		constexpr void put_tab( wchar_t, OutputIterator &oi ) {
			put_char( oi, L'\t' );
		}

		template<typename CharT, typename Traits, typename OutputIterator, typename Duration, typename... FormatFlags>
		constexpr void process_brace( daw::basic_string_view<CharT, Traits> &fmt_str, date::sys_time<Duration> const &tp,
		                              OutputIterator &oi, FormatFlags &&... flags ) {
			fmt_str.remove_prefix( 1 );
			auto const pos_last = fmt_str.find_first_of( static_cast<CharT>( '}' ) );
			if( pos_last == fmt_str.npos || pos_last == 0 ) {
				throw invalid_date_field{};
			}
			auto const idx = details::parse_unsigned<size_t>( fmt_str.substr( 0, pos_last ) );
			formats::get_string_value<CharT, Traits>( idx, tp, oi, std::forward<FormatFlags>( flags )... );
			fmt_str.remove_prefix( pos_last );
		}

		template<typename CharT, typename Traits, typename OutputIterator, typename Duration>
		constexpr void process_percent( daw::basic_string_view<CharT, Traits> &fmt_str, date::sys_time<Duration> const &tp,
		                                OutputIterator &oi ) {

			fmt_str.remove_prefix( 1 );
			int current_width = -1;
			enum class locale_modifiers { none, E, O };
			locale_modifiers locale_modifer{locale_modifiers::none};

			if( details::is_digit( fmt_str.front( ) ) ) {
				current_width = details::to_integer<int>( fmt_str.front( ) );
				fmt_str.remove_prefix( 1 );
				while( details::is_digit( fmt_str.front( ) ) ) {
					current_width *= 10;
					current_width = details::to_integer<int>( fmt_str.front( ) );
					fmt_str.remove_prefix( 1 );
				}
			} else if( fmt_str.front( ) == 'E' ) {
				locale_modifer = locale_modifiers::E;
				fmt_str.remove_prefix( 1 );
			} else if( fmt_str.front( ) == 'O' ) {
				locale_modifer = locale_modifiers::O;
				fmt_str.remove_prefix( 1 );
			}
			switch( fmt_str.front( ) ) {
			case '%':
				put_percent( CharT{}, oi );
				break;
			case 'a':
				formats::Day_of_Week<CharT, Traits>{formats::impl::locale_name_formats::abbreviated}( tp, oi );
				break;
			case 'A':
				formats::Day_of_Week<CharT, Traits>{formats::impl::locale_name_formats::full}( tp, oi );
				break;
			case 'b':
				formats::MonthName<CharT, Traits>{formats::impl::locale_name_formats::abbreviated}( tp, oi );
				break;
			case 'B':
				formats::MonthName<CharT, Traits>{formats::impl::locale_name_formats::full}( tp, oi );
				break;
			case 'c':
				formats::LocaleDateTime<CharT, Traits>{}( tp, oi );
				break;
			case 'C':
				formats::Century<CharT, Traits>{}( tp, oi );
				break;
			case 'D':
				default_width( current_width, 2 );
				formats::Month<CharT, Traits>{current_width}( tp, oi );
				put_char( oi, '/' );
				formats::Day<CharT, Traits>{current_width}( tp, oi );
				put_char( oi, '/' );
				formats::Year<CharT, Traits>{current_width}( tp, oi );
				break;
			case 'd':
			case 'e':
				default_width( current_width, 2 );
				formats::Day<CharT, Traits>{current_width}( tp, oi );
				break;
			case 'F':
				default_width( current_width, 4 );
				formats::Year<CharT, Traits>{current_width}( tp, oi );
				put_char( oi, '-' );
				formats::Month<CharT, Traits>{2}( tp, oi );
				put_char( oi, '-' );
				formats::Day<CharT, Traits>{2}( tp, oi );
				break;
			case 'g':
			case 'G':
			case 'h':
				throw unsupported_date_field{};
			case 'H':
				default_width( current_width, 2 );
				formats::Hour<CharT, Traits>{current_width}( tp, oi );
				break;
			case 'I':
				default_width( current_width, 2 );
				formats::Hour<CharT, Traits>{current_width, formats::hour_formats::twelve_hour}( tp, oi );
				break;
			case 'j':
				default_width( current_width, 3 );
				formats::Day_of_Year<CharT, Traits>{current_width}( tp, oi );
				break;
			case 'm':
				default_width( current_width, 2 );
				formats::Month<CharT, Traits>{current_width}( tp, oi );
				break;
			case 'M':
				default_width( current_width, 2 );
				formats::Minute<CharT, Traits>{current_width}( tp, oi );
				break;
			case 'n':
				put_newline( CharT{}, oi );
			case 't':
				put_tab( CharT{}, oi );
			case 'Y':
				if( locale_modifer == locale_modifiers::E ) {
					formats::Year<CharT, Traits>{-1, formats::impl::locale_name_formats::alternate}( tp, oi );
				} else {
					formats::Year<CharT, Traits>{ current_width }( tp, oi );
				}
				break;
			default:
				throw invalid_date_field{};
			}
		}
	} // namespace impl

	template<typename CharT, typename Traits, typename OutputIterator, typename Duration, typename... Flags>
	constexpr OutputIterator fmt( daw::basic_string_view<CharT, Traits> fmt_str, date::sys_time<Duration> const &tp,
	                              OutputIterator oi, Flags &&... flags ) {

		while( !fmt_str.empty( ) ) {
			switch( fmt_str.front( ) ) {
			case '%':
				impl::process_percent<CharT, Traits>( fmt_str, tp, oi );
				break;
			case '{':
				impl::process_brace<CharT, Traits>( fmt_str, tp, oi, std::forward<Flags>( flags )... );
				break;
			default:
				impl::put_char( oi, fmt_str.front( ) );
				break;
			}
			fmt_str.remove_prefix( 1 );
		}
		return oi;
	}

	template<typename CharT, size_t N, typename OutputIterator, typename Duration, typename... FormatFlags>
	constexpr OutputIterator fmt( CharT const ( &fmt_str )[N], date::sys_time<Duration> const &tp, OutputIterator oi,
	                              FormatFlags &&... flags ) {

		return fmt( daw::basic_string_view<CharT>{fmt_str, N}, tp, oi, flags... );
	}

	template<typename Duration>
	std::string strftime( daw::string_view format_str, date::sys_time<Duration> const &tp ) {
		std::string result{};
		auto oi = std::back_inserter( result );
		date::formats::impl::localize( char{}, tp, oi, format_str );
		return result;
	}

	template<typename Duration>
	std::wstring wcsftime( daw::wstring_view format_str, date::sys_time<Duration> const &tp ) {
		std::wstring result{};
		auto oi = std::back_inserter( result );
		date::formats::impl::localize( wchar_t{}, tp, oi, format_str );
		return result;
	}

} // namespace date
