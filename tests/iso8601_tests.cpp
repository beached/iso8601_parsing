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

#include "iso8601.h"

int main( ) {
	using namespace std::chrono;
	using namespace date;

	constexpr auto const dte = parse_iso8601_date( "2018-01-02" );
	std::cout << "2018-01-02 -> " << dte << '\n';
	constexpr auto const tme = parse_iso8601_time( "01:02:03.3434" );
	std::cout << "01:02:03.3434 -> " << tme << '\n';
	constexpr auto const tme2 = parse_iso8601_time( "010203.3434" );
	std::cout << "010203.3434 -> " << tme2<< '\n';
	static_assert( tme == tme2 );
	constexpr auto const tp = parse_iso8601_timestamp( "2018-01-02T01:02:03.3434Z" );
	std::cout << "2018-01-02T01:02:03.3434Z -> " << tp << '\n';
	constexpr auto const tp2 = parse_iso8601_timestamp( "2018-01-02T01:02:03.3434+0000" );
	std::cout << "2018-01-02T01:02:03.3434+0000 ->" << tp2 << '\n';
	constexpr auto const tp3 = parse_iso8601_timestamp( "20180102010203.3434Z" );
	std::cout << "2018-01-02T01:02:03.3434Z ->" << tp3 << '\n';
	static_assert( tp == tp2 );
	static_assert( tp2 == tp3 );
	return EXIT_SUCCESS;
}
