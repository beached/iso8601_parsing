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

#include <array>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <date/date.h>

#include <daw/cpp_17.h>
#include <daw/daw_bounded_string.h>
#include <daw/daw_string_view.h>
#include <daw/daw_utility.h>

#include "daw_common.h"

namespace date_formatting {
	struct invalid_date_field {};
	struct unsupported_date_field {};

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

		template<typename Duration>
		constexpr auto get_tod( date::sys_time<Duration> const &tp ) noexcept {
			auto const dte = date::floor<date::days>( tp );
			return date::make_time( tp - dte );
		}

		template<typename Duration, typename OutputIterator>
		struct fmt_state {
			date::sys_time<Duration> tp;
			OutputIterator oi;
			date::year_month_day ymd;
			time_t time;
			mutable std::optional<time_t> m_time{};

			using tod_t = decltype( get_tod( tp ) );

			tod_t tod;

			constexpr fmt_state( date::sys_time<Duration> t, OutputIterator i ) noexcept
			  : tp{std::move( t )}
			  , oi{std::move( i )}
			  , ymd{date::floor<date::days>( tp )}
			  , time{static_cast<time_t>( date::floor<std::chrono::seconds>( tp ).time_since_epoch( ).count( ) )}
			  , tod{get_tod( tp )} {}
		};

		template<typename Duration, typename OutputIterator>
		constexpr fmt_state<Duration, OutputIterator> make_state( date::sys_time<Duration> t, OutputIterator i ) noexcept {
			return fmt_state<Duration, OutputIterator>{std::move( t ), std::move( i )};
		}

		template<typename OutputIterator, typename CharT>
		constexpr void put_char( OutputIterator &oi, CharT c ) {
			*oi++ = c;
		}

		template<typename OutputIterator>
		constexpr void output_digit( char, OutputIterator &oi, uint8_t digit ) {
			date_formatting::impl::put_char( oi, '0' + digit );
		}

		template<typename OutputIterator>
		constexpr void output_digit( wchar_t, OutputIterator &oi, uint8_t digit ) {
			date_formatting::impl::put_char( oi, L'0' + digit );
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

		template<typename State>
		void localize( char, State &state, daw::string_view fmt ) {
			// Using string because of SSO and that explains the 15 char size.
			// The 30 increases is a WAG
			std::string buff( static_cast<size_t>( 15 ), '\0' );
			auto result = std::strftime( buff.data( ), buff.size( ), fmt.data( ), std::localtime( &state.time ) );
			while( result == 0 ) {
				buff.resize( buff.size( ) + 30 );
				result = std::strftime( buff.data( ), buff.size( ), fmt.data( ), std::localtime( &state.time ) );
			}
			state.oi = std::copy( buff.data( ), buff.data( ) + result, state.oi );
		}

		template<typename State>
		void localize( wchar_t, State &state, daw::wstring_view fmt ) {
			// Using string because of SSO and that explains the 7 wchar_t size.
			// The 30 increases is a WAG
			std::wstring buff( static_cast<size_t>( 7 ), L'\0' );
			auto result = std::wcsftime( buff.data( ), buff.size( ), fmt.data( ), std::localtime( &state.time ) );
			while( result == 0 ) {
				buff.resize( buff.size( ) + 30 );
				result = std::wcsftime( buff.data( ), buff.size( ), fmt.data( ), std::localtime( &state.time ) );
			}
			state.oi = std::copy( buff.data( ), buff.data( ) + result, state.oi );
		}
	} // namespace impl

	namespace formats {
		enum class locale_name_formats { abbreviated, full, none, alternate };

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Century {
			template<typename State>
			constexpr void operator( )( State &state ) const {
				auto yr = static_cast<int>( state.ymd.year( ) );
				yr /= 100;
				impl::output_digits( CharT{}, 2, state.oi, yr );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Year {
			int field_width = -1;
			locale_name_formats locale_name_format = locale_name_formats::none;

			template<typename State>
			constexpr void operator( )( State &state ) const {
				if( locale_name_format == locale_name_formats::alternate ) {
					impl::localize( CharT{}, state, "%EY" );
				} else {
					auto yr = static_cast<int>( state.ymd.year( ) );
					auto width = impl::format_width( field_width, yr );
					impl::output_digits( CharT{}, width, state.oi, yr );
				}
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct ISOWeekBasedYear {
			locale_name_formats locale_name_format = locale_name_formats::full;

			template<typename State>
			void operator( )( State &state ) const {
				if( locale_name_format == locale_name_formats::full ) {
					impl::localize( CharT{}, state, "%G" );
				} else {
					impl::localize( CharT{}, state, "%g" );
				}
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Month {
			int field_width = -1;

			template<typename State>
			constexpr void operator( )( State &state ) const {
				auto mo = static_cast<int>( static_cast<unsigned>( state.ymd.month( ) ) );
				if( field_width == 0 ) {
					impl::output_digits( CharT{}, 2, state.oi, mo - 1 );
				} else {
					impl::output_digits( CharT{}, 2, state.oi, mo );
				}
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Day {
			int field_width = -1;

			template<typename State>
			constexpr void operator( )( State &state ) const {
				auto dy = static_cast<int>( static_cast<unsigned>( state.ymd.day( ) ) );
				auto width = impl::format_width( field_width, dy );
				impl::output_digits( CharT{}, width, state.oi, dy );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Day_of_Week {
			locale_name_formats locale_name_format = locale_name_formats::full;

			template<typename State>
			void operator( )( State &state ) const {
				if( locale_name_format == locale_name_formats::full ) {
					impl::localize( CharT{}, state, "%A" );
				} else {
					impl::localize( CharT{}, state, "%a" );
				}
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct MonthName {
			locale_name_formats locale_name_format = locale_name_formats::full;

			template<typename State>
			void operator( )( State &state ) const {
				if( locale_name_format == locale_name_formats::full ) {
					impl::localize( CharT{}, state, "%B" );
				} else {
					impl::localize( CharT{}, state, "%b" );
				}
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct LocaleDateTime {
			template<typename State>
			void operator( )( State &state ) const {
				impl::localize( CharT{}, state, "%c" );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Day_of_Year {
			int field_width = -1;

			template<typename State>
			constexpr void operator( )( State &state ) const {
				auto const jan1 = state.ymd.year( ) / 1 / 1;
				auto diff = std::chrono::floor<date::days>( static_cast<date::sys_days>( state.ymd ) -
				                                            static_cast<date::sys_days>( jan1 ) )
				              .count( ) +
				            1;
				auto width = impl::format_width( field_width, diff );
				impl::output_digits( CharT{}, width, state.oi, diff );
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

			template<typename State>
			constexpr void operator( )( State &state ) const {
				auto hr = static_cast<int>( state.tod.hours( ).count( ) );
				if( hour_format == hour_formats::twelve_hour && hr >= 12 ) {
					hr -= 12;
				}
				auto width = impl::format_width( field_width, hr );
				impl::output_digits( CharT{}, width, state.oi, hr );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Minute {
			int field_width = -1;

			template<typename State>
			constexpr void operator( )( State &state ) const {
				auto mn = static_cast<int>( state.tod.minutes( ).count( ) );
				auto width = impl::format_width( field_width, mn );
				impl::output_digits( CharT{}, width, state.oi, mn );
			}
		};

		template<typename CharT = char, typename Traits = std::char_traits<CharT>>
		struct Second {
			int field_width = -1;

			template<typename State>
			constexpr void operator( )( State &state ) const {
				auto sc = static_cast<int>( state.tod.seconds( ).count( ) );
				auto width = impl::format_width( field_width, sc );
				impl::output_digits( CharT{}, width, state.oi, sc );
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

			template<typename State>
			constexpr void operator( )( State &state ) const {
				Year<CharT, Traits>{}( state );
				date_formatting::impl::put_char( state.oi, separator );
				Month<CharT, Traits>{}( state );
				date_formatting::impl::put_char( state.oi, separator );
				Day<CharT, Traits>{}( state );
			}
		};

		template<typename CharT, typename Traits, size_t MaxLen = 100>
		struct MonthDayYear {
			int field_width = -1;
			CharT separator = '/';

			template<typename State>
			constexpr void operator( )( State &state ) const {
				Month<CharT, Traits>{field_width}( state );
				::date_formatting::impl::put_char( state.oi, separator );
				Day<CharT, Traits>{field_width}( state );
				::date_formatting::impl::put_char( state.oi, separator );
				Year<CharT, Traits>{field_width}( state );
			}
		};
		// Get nth value from a parameter pack
		namespace impl {
			template<typename CharT, typename Traits, typename State, typename Arg>
			constexpr auto runarg( State &state, Arg &&arg ) -> std::enable_if_t<daw::traits::is_callable_v<Arg, State &>> {
				arg( state );
			}

			template<typename CharT, typename Traits, typename State, typename Arg>
			constexpr auto runarg( State &state, Arg &&arg ) -> std::enable_if_t<!daw::traits::is_callable_v<Arg, State &>> {
				std::basic_string<CharT, Traits> result = arg( );
				state.oi = std::copy( result.cbegin( ), result.cend( ), state.oi );
			}

			template<size_t index, typename CharT, typename Traits, typename State>
			constexpr void get_string_value( size_t const, State & ) noexcept {}

			template<size_t index, typename CharT, typename Traits, typename State, typename Arg, typename... Args>
			constexpr void get_string_value( size_t const n, State &state, Arg &&arg, Args &&... args ) {
				if( index == n ) {
					runarg<CharT, Traits>( state, std::forward<Arg>( arg ) );
				} else {
					get_string_value<index + 1, CharT, Traits>( n, state, std::forward<Args>( args )... );
				}
			}
		} // namespace impl

		template<typename CharT, typename Traits, typename State, typename... Args>
		constexpr void get_string_value( size_t const n, State &state, Args &&... args ) {
			if( n >= sizeof...( Args ) ) {
				throw std::out_of_range{"Invalid index to parameter pack"};
			}
			impl::get_string_value<0, CharT, Traits>( n, state, std::forward<Args>( args )... );
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

		template<typename T, typename U>
		constexpr void default_width( T &value, U def_value ) noexcept {
			if( value < 1 ) {
				value = static_cast<T>( def_value );
			}
		}

		template<typename CharT, typename Traits, typename State, typename... FormatFlags>
		constexpr void process_brace( daw::basic_string_view<CharT, Traits> &fmt_str, State &state,
		                              FormatFlags &&... flags ) {
			fmt_str.remove_prefix( 1 );
			auto const pos_last = fmt_str.find_first_of( static_cast<CharT>( '}' ) );
			if( pos_last == fmt_str.npos || pos_last == 0 ) {
				throw invalid_date_field{};
			}
			auto const idx = daw::details::parse_unsigned<size_t>( fmt_str.substr( 0, pos_last ) );
			formats::get_string_value<CharT, Traits>( idx, state, std::forward<FormatFlags>( flags )... );
			fmt_str.remove_prefix( pos_last );
		}

		template<typename CharT, typename Traits, typename State>
		constexpr void process_percent( daw::basic_string_view<CharT, Traits> &fmt_str, State &state ) {
			fmt_str.remove_prefix( 1 );
			int current_width = -1;
			enum class locale_modifiers { none, E, O };
			locale_modifiers locale_modifer{locale_modifiers::none};

			if( daw::details::is_digit( fmt_str.front( ) ) ) {
				current_width = daw::details::to_integer<int>( fmt_str.front( ) );
				fmt_str.remove_prefix( 1 );
				while( daw::details::is_digit( fmt_str.front( ) ) ) {
					current_width *= 10;
					current_width = daw::details::to_integer<int>( fmt_str.front( ) );
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
				put_percent( CharT{}, state.oi );
				break;
			case 'a':
				formats::Day_of_Week<CharT, Traits>{formats::locale_name_formats::abbreviated}( state );
				break;
			case 'A':
				formats::Day_of_Week<CharT, Traits>{formats::locale_name_formats::full}( state );
				break;
			case 'b':
			case 'h':
				formats::MonthName<CharT, Traits>{formats::locale_name_formats::abbreviated}( state );
				break;
			case 'B':
				formats::MonthName<CharT, Traits>{formats::locale_name_formats::full}( state );
				break;
			case 'c':
				formats::LocaleDateTime<CharT, Traits>{}( state );
				break;
			case 'C':
				formats::Century<CharT, Traits>{}( state );
				break;
			case 'D':
				default_width( current_width, 2 );
				formats::Month<CharT, Traits>{current_width}( state );
				put_char( state.oi, '/' );
				formats::Day<CharT, Traits>{current_width}( state );
				put_char( state.oi, '/' );
				formats::Year<CharT, Traits>{current_width}( state );
				break;
			case 'd':
			case 'e':
				default_width( current_width, 2 );
				formats::Day<CharT, Traits>{current_width}( state );
				break;
			case 'F':
				default_width( current_width, 4 );
				formats::Year<CharT, Traits>{current_width}( state );
				put_char( state.oi, '-' );
				formats::Month<CharT, Traits>{2}( state );
				put_char( state.oi, '-' );
				formats::Day<CharT, Traits>{2}( state );
				break;
			case 'g':
				formats::ISOWeekBasedYear<CharT, Traits>{}( state );
				break;
			case 'G':
				formats::ISOWeekBasedYear<CharT, Traits>{formats::locale_name_formats::abbreviated}( state );
				break;
			case 'H':
				default_width( current_width, 2 );
				formats::Hour<CharT, Traits>{current_width}( state );
				break;
			case 'I':
				default_width( current_width, 2 );
				formats::Hour<CharT, Traits>{current_width, formats::hour_formats::twelve_hour}( state );
				break;
			case 'j':
				default_width( current_width, 3 );
				formats::Day_of_Year<CharT, Traits>{current_width}( state );
				break;
			case 'm':
				default_width( current_width, 2 );
				formats::Month<CharT, Traits>{current_width}( state );
				break;
			case 'M':
				default_width( current_width, 2 );
				formats::Minute<CharT, Traits>{current_width}( state );
				break;
			case 'n':
				put_newline( CharT{}, state.oi );
			case 't':
				put_tab( CharT{}, state.oi );
			case 'Y':
				if( locale_modifer == locale_modifiers::E ) {
					formats::Year<CharT, Traits>{-1, formats::locale_name_formats::alternate}( state );
				} else {
					formats::Year<CharT, Traits>{current_width}( state );
				}
				break;
			default:
				throw invalid_date_field{};
			}
		}
	} // namespace impl

	template<typename CharT, typename Traits, typename State, typename... Flags>
	constexpr auto fmt( daw::basic_string_view<CharT, Traits> fmt_str, State &state, Flags &&... flags ) {

		while( !fmt_str.empty( ) ) {
			switch( fmt_str.front( ) ) {
			case '%':
				impl::process_percent<CharT, Traits>( fmt_str, state );
				break;
			case '{':
				impl::process_brace<CharT, Traits>( fmt_str, state, std::forward<Flags>( flags )... );
				break;
			default:
				impl::put_char( state.oi, fmt_str.front( ) );
				break;
			}
			fmt_str.remove_prefix( 1 );
		}
		return state.oi;
	}

	template<typename CharT, size_t N, typename OutputIterator, typename Duration, typename... FormatFlags>
	constexpr OutputIterator fmt( CharT const ( &fmt_str )[N], date::sys_time<Duration> const &tp, OutputIterator oi,
	                              FormatFlags &&... flags ) {
		auto state = impl::make_state( tp, oi );
		return fmt( daw::basic_string_view<CharT>{fmt_str, N - 1}, state, flags... );
	}

	template<typename Duration, typename... FormatFlags>
	std::string fmt_string( daw::string_view format_str, date::sys_time<Duration> const &tp, FormatFlags &&... flags ) {
		std::string result{};
		auto state = impl::make_state( tp, std::back_inserter( result ) );
		fmt( format_str, state, std::forward<FormatFlags>( flags )... );
		return result;
	}

	template<typename Duration, typename... FormatFlags>
	std::wstring fmt_string( daw::wstring_view format_str, date::sys_time<Duration> const &tp, FormatFlags &&... flags ) {
		std::wstring result{};
		auto state = impl::make_state( tp, std::back_inserter( result ) );
		fmt( format_str, state, std::forward<FormatFlags>( flags )... );
		return result;
	}

	template<typename CharT, size_t N, typename OutputIterator, typename Duration, typename... FormatFlags>
	auto fmt_string( CharT const ( &fmt_str )[N], date::sys_time<Duration> const &tp, FormatFlags &&... flags ) {
		return fmt_string( {fmt_str, N - 1}, tp, std::forward<FormatFlags>( flags )... );
	}

	template<typename CharT, typename Traits, typename Duration, typename... FormatFlags>
	std::basic_ostream<CharT, Traits> &fmt_stream( daw::basic_string_view<CharT, Traits> format_str,
	                                               date::sys_time<Duration> const &tp,
	                                               std::basic_ostream<CharT, Traits> &ostr, FormatFlags &&... flags ) {

		auto state = impl::make_state( tp, std::ostream_iterator<CharT>{ostr} );
		fmt( format_str, state, std::forward<FormatFlags>( flags )... );
		return ostr;
	}

	template<typename CharT, size_t N, typename Traits, typename Duration, typename... FormatFlags>
	std::basic_ostream<CharT, Traits> &fmt_stream( CharT const ( &format_str )[N], date::sys_time<Duration> const &tp,
	                                               std::basic_ostream<CharT, Traits> &ostr, FormatFlags &&... flags ) {

		auto state = impl::make_state( tp, std::ostream_iterator<CharT>{ostr} );
		fmt( daw::basic_string_view<CharT, Traits>{format_str, N - 1}, state, std::forward<FormatFlags>( flags )... );
		return ostr;
	}

	template<typename Duration>
	std::string strftime( daw::string_view format_str, date::sys_time<Duration> const &tp ) {
		std::string result{};
		auto state = impl::make_state( tp, std::back_inserter( result ) );
		impl::localize( char{}, state, format_str );
		return result;
	}

	template<typename Duration>
	std::wstring wcsftime( daw::wstring_view format_str, date::sys_time<Duration> const &tp ) {
		std::wstring result{};
		auto state = impl::make_state( tp, std::back_inserter( result ) );
		impl::localize( wchar_t{}, state, format_str );
		return result;
	}

	namespace impl {
		template<typename CharT, size_t MaxLen>
		struct StringData {
			daw::basic_bounded_string<CharT, MaxLen> data;

			template<typename State>
			constexpr void operator( )( State &state ) const {
				impl::copy( data.cbegin( ), data.cend( ), state.oi );
			}
		};

		constexpr bool is_escape_symbol( char value ) noexcept {
			return value == '%' || value == '{';
		}

		constexpr bool is_escape_symbol( wchar_t value ) noexcept {
			return value == L'%' || value == L'{';
		}

		template<typename CharT, size_t MaxLen>
		constexpr daw::basic_bounded_string<CharT, MaxLen> parse_string( daw::string_view &fmt_str ) noexcept {
			daw::basic_bounded_string<CharT, MaxLen> result{};
			while( !fmt_str.empty( ) && result.has_room( 1 ) ) {
				if( is_escape_symbol( fmt_str.front( ) ) ) {
					break;
				}
				result.push_back( fmt_str.front( ) );
				fmt_str.remove_prefix( 1 );
			}
			return result;
		}

		template<typename CharT, typename Traits>
		struct IndexedFlag {
			size_t index = 0;

			template<typename State, typename... FormatFlags>
			constexpr void operator( )( State &state, FormatFlags &&... flags ) const {
				formats::get_string_value<CharT, Traits>( index, state, std::forward<FormatFlags>( flags )... );
			}
		};

		template<typename CharT, typename Traits>
		constexpr IndexedFlag<CharT, Traits> process_brace2( daw::basic_string_view<CharT, Traits> &fmt_str ) {
			fmt_str.remove_prefix( 1 );
			auto const pos_last = fmt_str.find_first_of( static_cast<CharT>( '}' ) );
			if( pos_last == fmt_str.npos || pos_last == 0 ) {
				throw invalid_date_field{};
			}
			auto const idx = daw::details::parse_unsigned<size_t>( fmt_str.substr( 0, pos_last ) );
			fmt_str.remove_prefix( pos_last );
			return IndexedFlag<CharT, Traits>{idx};
		}

		template<size_t MaxLen, typename Result, typename CharT, typename Traits>
		constexpr void process_percent2( daw::basic_string_view<CharT, Traits> &fmt_str, Result &result ) {
			fmt_str.remove_prefix( 1 );
			int current_width = -1;
			enum class locale_modifiers { none, E, O };
			locale_modifiers locale_modifer{locale_modifiers::none};

			if( daw::details::is_digit( fmt_str.front( ) ) ) {
				current_width = daw::details::to_integer<int>( fmt_str.front( ) );
				fmt_str.remove_prefix( 1 );
				while( daw::details::is_digit( fmt_str.front( ) ) ) {
					current_width *= 10;
					current_width = daw::details::to_integer<int>( fmt_str.front( ) );
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
				result = StringData<CharT, MaxLen>{daw::basic_bounded_string<CharT, MaxLen>{"%"}};
				break;
			case 'a':
				result = formats::Day_of_Week<CharT, Traits>{formats::locale_name_formats::abbreviated};
				break;
			case 'A':
				result = formats::Day_of_Week<CharT, Traits>{formats::locale_name_formats::full};
				break;
			case 'b':
			case 'h':
				result = formats::MonthName<CharT, Traits>{formats::locale_name_formats::abbreviated};
				break;
			case 'B':
				result = formats::MonthName<CharT, Traits>{formats::locale_name_formats::full};
				break;
			case 'c':
				result = formats::LocaleDateTime<CharT, Traits>{};
				break;
			case 'C':
				result = formats::Century<CharT, Traits>{};
				break;
			case 'D':
				default_width( current_width, 2 );
				result = formats::MonthDayYear<CharT, Traits, MaxLen>{current_width};
				break;
			case 'd':
			case 'e':
				default_width( current_width, 2 );
				result = formats::Day<CharT, Traits>{current_width};
				break;
			case 'F':
				result = formats::YearMonthDay<CharT, Traits>{'-'};
				break;
			case 'g':
				result = formats::ISOWeekBasedYear<CharT, Traits>{};
				break;
			case 'G':
				result = formats::ISOWeekBasedYear<CharT, Traits>{formats::locale_name_formats::abbreviated};
				break;
			case 'H':
				default_width( current_width, 2 );
				result = formats::Hour<CharT, Traits>{current_width};
				break;
			case 'I':
				default_width( current_width, 2 );
				result = formats::Hour<CharT, Traits>{current_width, formats::hour_formats::twelve_hour};
				break;
			case 'j':
				default_width( current_width, 3 );
				result = formats::Day_of_Year<CharT, Traits>{current_width};
				break;
			case 'm':
				default_width( current_width, 2 );
				result = formats::Month<CharT, Traits>{current_width};
				break;
			case 'M':
				default_width( current_width, 2 );
				result = formats::Minute<CharT, Traits>{current_width};
				break;
			case 'n':
				result = StringData<CharT, MaxLen>{daw::basic_bounded_string<CharT, MaxLen>{"\n"}};
				break;
			case 't':
				result = StringData<CharT, MaxLen>{daw::basic_bounded_string<CharT, MaxLen>{"\t"}};
				break;
			case 'Y':
				if( locale_modifer == locale_modifiers::E ) {
					result = formats::Year<CharT, Traits>{-1, formats::locale_name_formats::alternate};
				} else {
					result = formats::Year<CharT, Traits>{current_width};
				}
				break;
			default:
				throw invalid_date_field{};
			}
			fmt_str.remove_prefix( 1 );
		}

		template<typename CharT, typename Traits, typename... Args>
		struct fmt_visitor {
			mutable std::tuple<Args...> args;

			template<typename T>
			constexpr void operator( )( T const &obj ) const {
				obj( std::get<0>( args ) );
			}

			constexpr void operator( )( IndexedFlag<CharT, Traits> const &t ) const {
				std::apply( t, args );
			}
		};

		template<typename CharT, typename Traits, typename... Args>
		constexpr auto make_fmt_visitor( Args &&... args ) {
			return fmt_visitor<CharT, Traits, Args...>{std::forward_as_tuple( std::forward<Args>( args )... )};
		}

		template<typename CharT, typename Traits, size_t MaxStringLen, typename T>
		constexpr size_t date_field_index = daw::pack_index_of_v<
		  T, formats::Century<CharT, Traits>, formats::Year<CharT, Traits>, formats::ISOWeekBasedYear<CharT, Traits>,
		  formats::Month<CharT, Traits>, formats::Day<CharT, Traits>, formats::Day_of_Week<CharT, Traits>,
		  formats::MonthName<CharT, Traits>, formats::LocaleDateTime<CharT, Traits>, formats::Day_of_Year<CharT, Traits>,
		  formats::Hour<CharT, Traits>, formats::Minute<CharT, Traits>, formats::Second<CharT, Traits>,
		  formats::YearMonthDay<CharT, Traits>, impl::StringData<CharT, MaxStringLen>, impl::IndexedFlag<CharT, Traits>,
		  formats::MonthDayYear<CharT, Traits, MaxStringLen>>;

		template<typename CharT, typename Traits, size_t MaxStringLen = 100>
		struct date_formatter_storage_t {
			struct empty_t {};
			static constexpr size_t const empty_idx = std::numeric_limits<size_t>::max( );

			union value_t {
				empty_t empty;
				formats::Century<CharT, Traits> val_century;
				formats::Year<CharT, Traits> val_year;
				formats::ISOWeekBasedYear<CharT, Traits> val_iso_week_based_year;
				formats::Month<CharT, Traits> val_month;
				formats::Day<CharT, Traits> val_day;
				formats::Day_of_Week<CharT, Traits> val_day_of_week;
				formats::MonthName<CharT, Traits> val_month_name;
				formats::LocaleDateTime<CharT, Traits> val_locale_date_time;
				formats::Day_of_Year<CharT, Traits> val_day_of_year;
				formats::Hour<CharT, Traits> val_hour;
				formats::Minute<CharT, Traits> val_minute;
				formats::Second<CharT, Traits> val_second;
				formats::YearMonthDay<CharT, Traits> val_year_month_day;
				impl::StringData<CharT, MaxStringLen> val_string_data;
				impl::IndexedFlag<CharT, Traits> val_indexed_flag;
				formats::MonthDayYear<CharT, Traits, MaxStringLen> val_month_day_year;

				constexpr value_t( ) noexcept
				  : empty( ) {}

				constexpr value_t( formats::Century<CharT, Traits> const &cent ) noexcept
				  : val_century( cent ) {}

				constexpr value_t( formats::Year<CharT, Traits> const &yr ) noexcept
				  : val_year( yr ) {}

				constexpr value_t( formats::ISOWeekBasedYear<CharT, Traits> const &iso_wk_year ) noexcept
				  : val_iso_week_based_year( iso_wk_year ) {}

				constexpr value_t( formats::Month<CharT, Traits> const &mo ) noexcept
				  : val_month( mo ) {}

				constexpr value_t( formats::Day<CharT, Traits> const &dy ) noexcept
				  : val_day( dy ) {}

				constexpr value_t( formats::Day_of_Week<CharT, Traits> const &dw ) noexcept
				  : val_day_of_week( dw ) {}

				constexpr value_t( formats::MonthName<CharT, Traits> const &mo ) noexcept
				  : val_month_name( mo ) {}

				constexpr value_t( formats::LocaleDateTime<CharT, Traits> const &loc ) noexcept
				  : val_locale_date_time( loc ) {}

				constexpr value_t( formats::Day_of_Year<CharT, Traits> const &doy ) noexcept
				  : val_day_of_year( doy ) {}

				constexpr value_t( formats::Hour<CharT, Traits> const &hr ) noexcept
				  : val_hour( hr ) {}

				constexpr value_t( formats::Minute<CharT, Traits> const &min ) noexcept
				  : val_minute( min ) {}

				constexpr value_t( formats::Second<CharT, Traits> const &sc ) noexcept
				  : val_second( sc ) {}

				constexpr value_t( formats::YearMonthDay<CharT, Traits> const &ymd ) noexcept
				  : val_year_month_day( ymd ) {}

				constexpr value_t( impl::StringData<CharT, MaxStringLen> const &str ) noexcept
				  : val_string_data( str ) {}

				constexpr value_t( impl::IndexedFlag<CharT, Traits> const &idx ) noexcept
				  : val_indexed_flag( idx ) {}

				constexpr value_t( formats::MonthDayYear<CharT, Traits, MaxStringLen> const &mdy ) noexcept
				  : val_month_day_year( mdy ) {}
			};
			value_t value;
			size_t idx;

			constexpr date_formatter_storage_t( ) noexcept
			  : value( )
			  , idx( empty_idx ) {}

			template<typename T>
			constexpr date_formatter_storage_t( T &&date_field ) noexcept
			  : value( std::forward<T>( date_field ) )
			  , idx( date_field_index<CharT, Traits, MaxStringLen, T> ) {}

			template<typename T>
			constexpr void emplace( T &&date_field ) noexcept {
				value = value_t( std::forward<T>( date_field ) );
				idx = date_field_index<CharT, Traits, MaxStringLen, T>;
			}

			template<typename State, typename... Args>
			constexpr void operator( )( State &state, Args &&... args ) {
				switch( idx ) {
				case date_field_index<CharT, Traits, MaxStringLen, formats::Century<CharT, Traits>>:
					value.val_century( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::Year<CharT, Traits>>:
					value.val_year( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::ISOWeekBasedYear<CharT, Traits>>:
					value.val_iso_week_based_year( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::Month<CharT, Traits>>:
					value.val_month( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::Day<CharT, Traits>>:
					value.val_day( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::Day_of_Week<CharT, Traits>>:
					value.val_day_of_week( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::MonthName<CharT, Traits>>:
					value.val_month_name( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::LocaleDateTime<CharT, Traits>>:
					value.val_locale_date_time( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::Day_of_Year<CharT, Traits>>:
					value.val_day_of_year( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::Hour<CharT, Traits>>:
					value.val_hour( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::Minute<CharT, Traits>>:
					value.val_minute( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::Second<CharT, Traits>>:
					value.val_second( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::YearMonthDay<CharT, Traits>>:
					value.val_year_month_day( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, impl::StringData<CharT, MaxStringLen>>:
					value.val_string_data( state );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, impl::IndexedFlag<CharT, Traits>>:
					value.val_indexed_flag( state, std::forward<Args>( args )... );
					break;
				case date_field_index<CharT, Traits, MaxStringLen, formats::MonthDayYear<CharT, Traits, MaxStringLen>>:
					value.val_month_day_year( state );
					break;
				}
			}
		};
	} // namespace impl

	template<typename CharT, typename Traits = std::char_traits<CharT>, size_t MaxStringLen = 100>
	struct date_formatter_t {
		using fixed_string = daw::basic_bounded_string<CharT, MaxStringLen>;

		std::array<impl::date_formatter_storage_t<CharT, Traits, MaxStringLen>, 100> formatters{};
		size_t pos = 0;

		constexpr date_formatter_t( daw::basic_string_view<CharT, Traits> fmt_str ) {

			while( !fmt_str.empty( ) ) {
				switch( fmt_str.front( ) ) {
				case '%':
					impl::process_percent2<MaxStringLen>( fmt_str, formatters[pos++] );
					break;
				case '{': {
					formatters[pos++].template emplace<impl::IndexedFlag<CharT, Traits>>( impl::process_brace2( fmt_str ) );
				} break;
				default:
					formatters[pos++].template emplace<impl::StringData<CharT, MaxStringLen>>(
					  impl::StringData<CharT, MaxStringLen>{impl::parse_string<CharT, MaxStringLen>( fmt_str )} );
					break;
				}
				fmt_str.remove_prefix( 1 );
			}
		}

		template<typename Duration, typename OutputIterator, typename... FormatFlags>
		constexpr OutputIterator operator( )( date::sys_time<Duration> const &tp, OutputIterator oi,
		                                      FormatFlags &&... flags ) {
			auto state = impl::make_state( tp, oi );
			for( size_t n = 0; n < pos; ++n ) {
				formatters[n]( state, std::forward<FormatFlags>( flags )... );
			}
			return state.oi;
		}
	};
} // namespace date_formatting

