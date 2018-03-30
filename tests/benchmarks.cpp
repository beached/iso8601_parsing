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

#include <cstdlib>
#include <date/date.h>
#include <string>
#include <string_view>
#include <vector>

#include <daw/daw_benchmark.h>

#include "iso8601.h"

date::sys_time<std::chrono::milliseconds> parse8601( std::string const &ts ) {
	std::istringstream in{ts};
	date::sys_time<std::chrono::milliseconds> tp;
	in >> date::parse( "%FT%TZ", tp );
	if( in.fail( ) ) {
		in.clear( );
		in.exceptions( std::ios::failbit );
		in.str( ts );
		in >> date::parse( "%FT%T%Ez", tp );
	}
	return tp;
}

int main( ) {
	auto const bench_iso8601_parser = []( std::vector<std::string> const &timestamps ) {
		date::sys_time<std::chrono::milliseconds> result{std::chrono::milliseconds{0}};
		for( auto const &ts : timestamps ) {
			result += parse_iso8601_timestamp( ts ).time_since_epoch( );
		}
		return result;
	};

	auto const bench_iso8601_parser2 = []( std::vector<std::string> const &timestamps ) {
		date::sys_time<std::chrono::milliseconds> result{std::chrono::milliseconds{0}};
		for( auto const &ts : timestamps ) {
			result += parse8601( ts ).time_since_epoch( );
		}
		return result;
	};

	std::vector<std::string> timestamps{};
	for( size_t n = 0; n < 10'000; ++n ) {
		timestamps.emplace_back( "20170102T01:02:03.4567Z" );
	}

	daw::bench_test( "parse_iso8601_timestamp", bench_iso8601_parser, timestamps );
	daw::bench_test( "parse_iso8601_timestamp2", bench_iso8601_parser2, timestamps );

	return EXIT_SUCCESS;
}
