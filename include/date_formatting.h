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
#include <memory>
#include <string>
#include <vector>

#include <date/date.h>
#include <daw/cpp_17.h>
#include <daw/daw_static_optional.h>
#include <daw/daw_string_view.h>

#include "common.h"

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
			mutable daw::static_optional<time_t> m_time;

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

		// Get nth value from a parameter pack
		namespace impl {
			template<typename CharT, typename Traits, typename State, typename Arg>
			constexpr auto runarg( State &state, Arg &&arg ) -> std::enable_if_t<daw::is_callable_v<Arg, State &>> {
				arg( state );
			}

			template<typename CharT, typename Traits, typename State, typename Arg>
			constexpr auto runarg( State &state, Arg &&arg ) -> std::enable_if_t<!daw::is_callable_v<Arg, State &>> {
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
		return fmt( daw::basic_string_view<CharT>{fmt_str, N}, state, flags... );
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
		return fmt_string( {fmt_str, N}, tp, std::forward<FormatFlags>( flags )... );
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
		fmt( daw::basic_string_view<CharT, Traits>{format_str, N}, state, std::forward<FormatFlags>( flags )... );
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

} // namespace date_formatting
