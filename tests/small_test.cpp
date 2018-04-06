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
#include <iostream>
#include <iterator>

#include "date_formatting.h"
#include "iso8601_timestamps.h"

extern std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>
small_test( std::string_view ts_str ) {
	return date::parse_javascript_timestamp( ts_str );
}

constexpr auto testcx( std::string_view js_time ) {
	struct result_t {
		char value[21];
	} result{{0}};

	auto const tp01 = date::parse_javascript_timestamp( js_time );
	using namespace date::formats;
	char *ptr = result.value;
	date::fmt( "{0}T{1}:{2}:{3}\n", tp01, ptr, YearMonthDay{}, Hour{}, Minute{}, Second{} );
	return result;
}

constexpr auto testcx2( std::string_view js_time ) {
	struct result_t {
		char value[21];
	} result{{0}};

	auto const tp01 = date::parse_javascript_timestamp( js_time );
	using namespace date::formats;
	char *ptr = result.value;
	date::fmt( "%C %D\n", tp01, ptr );
	return result;
}

int main( ) {
	using namespace std::chrono;
	using namespace date;

	auto const tp01 = small_test( "2016-12-31T01:02:03.343Z" );
	std::cout << "2018-01-02T01:02:03.343Z -> " << tp01 << '\n';

	std::ostream_iterator<char> oi{std::cout};
	using namespace date::formats;
	std::cout << "fmt with format classes\n";
	date::fmt( "{0}T{1}:{2}:{3} DOW->{4}\n", tp01, oi, YearMonthDay{}, Hour{}, Minute{}, Second{}, Day_of_Week{} );

	constexpr auto const date_str = testcx( "2018-01-02T01:02:04.343Z" );
	std::cout << date_str.value;

	std::cout << "fmt with format string\n";
	constexpr auto const date_str2 = testcx2( "2018-01-02T13:02:04.343Z" );
	std::cout << date_str2.value;
	date::fmt( "%C %d %1d %2d %3d %D %F %1F %2F %3F %4F %5F %I %H %j %a %A %b %B %c\n", tp01,
	           std::ostream_iterator<char>{std::cout} );

	return EXIT_SUCCESS;
}
