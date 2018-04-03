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
#include <string_view>

#include "common.h"

namespace date {
	struct invalid_date_field {};

	namespace formats {
		struct Year;
		struct Month;
		struct Month_Name;
		struct Day;
		struct Day_of_Week;
		struct Hour;
		struct Minute;
		struct Second;
		struct UTCOffset;
		struct TimeZoneName;

		// Get nth value from a parameter pack
		namespace impl {
			template<size_t index, typename CharT, typename Traits, typename Duration, typename OutputIterator>
			constexpr void get_string_value( size_t const, date::sys_time<Duration> const &, OutputIterator & ) noexcept {}

			template<size_t index, typename CharT, typename Traits, typename Duration, typename OutputIterator, typename Arg,
			         typename... Args>
			constexpr void get_string_value( size_t const n, date::sys_time<Duration> const &tp, OutputIterator &oi,
			                                 Arg &&arg, Args &&... args ) {
				if( index == n ) {
					arg.template view<CharT, Traits>( tp, oi );
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
	}
	template<typename CharT, typename Traits, typename OutputIterator, typename Duration, typename... FormatFlags>
	constexpr OutputIterator fmt( std::basic_string_view<CharT, Traits> fmt_str, date::sys_time<Duration> const &tp,
	                              OutputIterator &oi, FormatFlags &&... flags ) {
		auto pos_first = fmt_str.find_first_of( static_cast<CharT>( '{' ) );
		while( pos_first != fmt_str.npos ) {
			if( pos_first == fmt_str.npos ) {
				oi = impl::copy( fmt_str.cbegin( ), fmt_str.cend( ), oi );
				fmt_str = std::basic_string_view<CharT, Traits>{};
				break;
			} else if( pos_first != 0 ) {
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

	template<typename CharT, size_t N, typename OutputIterator, typename Duration, typename... FormatFlags>
	constexpr OutputIterator fmt( CharT const ( &fmt_str )[N], date::sys_time<Duration> const &tp, OutputIterator oi,
	                              FormatFlags &&... flags ) {

		return fmt( std::basic_string_view<CharT>{fmt_str, N}, tp, oi, flags... );
	}

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
			constexpr Result pow10( size_t width ) noexcept {
				Result result = 1;
				while( width > 1 ) {
					result *= 10;
					--width;
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
		} // namespace impl

		struct Year {
			template<typename CharT, typename Traits, typename Duration, typename OutputIterator>
			constexpr void view( date::sys_time<Duration> const &tp, OutputIterator &oi ) {
				auto const yr = static_cast<int>( date::year_month_day{date::floor<date::days>( tp )}.year( ) );
				impl::output_digits( CharT{}, 4, oi, yr );
			}
		};

		struct Month {
			template<typename CharT, typename Traits, typename Duration, typename OutputIterator>
			constexpr void view( date::sys_time<Duration> const &tp, OutputIterator &oi ) {
				auto const mo = static_cast<unsigned>( date::year_month_day{date::floor<date::days>( tp )}.month( ) );
				impl::output_digits( CharT{}, 2, oi, mo );
			}
		};

		struct Month_Name;
		struct Day {
			template<typename CharT, typename Traits, typename Duration, typename OutputIterator>
			constexpr void view( date::sys_time<Duration> const &tp, OutputIterator &oi ) {
				auto const dy = static_cast<unsigned>( date::year_month_day{date::floor<date::days>( tp )}.day( ) );
				impl::output_digits( CharT{}, 2, oi, dy );
			}
		};

		struct Day_of_Week;
		struct Hour {
			template<typename CharT, typename Traits, typename Duration, typename OutputIterator>
			constexpr void view( date::sys_time<Duration> const &tp, OutputIterator &oi ) {
				auto const dte = date::floor<date::days>( tp );
				auto const tod = date::make_time( tp - dte );
				impl::output_digits( CharT{}, 2, oi, tod.hours( ).count( ) );
			}
		};
		struct Minute {
			template<typename CharT, typename Traits, typename Duration, typename OutputIterator>
			constexpr void view( date::sys_time<Duration> const &tp, OutputIterator &oi ) {
				auto const dte = date::floor<date::days>( tp );
				auto const tod = date::make_time( tp - dte );
				impl::output_digits( CharT{}, 2, oi, tod.minutes( ).count( ) );
			}
		};
		struct Second {
			template<typename CharT, typename Traits, typename Duration, typename OutputIterator>
			constexpr void view( date::sys_time<Duration> const &tp, OutputIterator &oi ) {
				auto const dte = date::floor<date::days>( tp );
				auto const tod = date::make_time( tp - dte );
				impl::output_digits( CharT{}, 2, oi, tod.seconds( ).count( ) );
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

		template<typename CharT = char>
		struct YearMonthDay {
			CharT separator;

			constexpr YearMonthDay( ) noexcept
			  : separator{impl::default_separator( CharT{} )} {}

			constexpr YearMonthDay( CharT sep ) noexcept
			  : separator{sep} {}

			constexpr YearMonthDay( YearMonthDay const & ) noexcept = default;

			constexpr YearMonthDay( YearMonthDay && ) noexcept = default;

			constexpr YearMonthDay &operator=( YearMonthDay const & ) noexcept = default;

			constexpr YearMonthDay &operator=( YearMonthDay && ) noexcept = default;

			~YearMonthDay( ) noexcept = default;

			template<typename CharT2, typename Traits, typename Duration, typename OutputIterator>
			constexpr void view( date::sys_time<Duration> const &tp, OutputIterator &oi ) {
				static_assert( std::is_same_v<CharT, CharT2>, "CharT on class must match CharT2 on view method" );
				Year{}.template view<CharT, Traits>( tp, oi );
				*oi = separator;
				++oi;
				Month{}.template view<CharT, Traits>( tp, oi );
				*oi = separator;
				++oi;
				Day{}.template view<CharT, Traits>( tp, oi );
			}
		};
	} // namespace formats
} // namespace date
