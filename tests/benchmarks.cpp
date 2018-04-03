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
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <daw/daw_benchmark.h>

#include "iso8601_timestamps.h"

date::sys_time<std::chrono::milliseconds> parse8601( std::string const &ts ) {
	std::istringstream in{ts};
	date::sys_time<std::chrono::milliseconds> tp;
	in >> date::parse( "%FT%TZ", tp );
	if( in.fail( ) ) {
		in.clear( );
		in.exceptions( std::ios::failbit );
		in.str( ts );
		in >> date::parse( "%FT%T%z", tp );
		if( in.fail( ) ) {
			std::cerr << "Unknown timestamp format: " << ts << '\n';
			throw date::invalid_iso8601_timestamp{};
		}
	}
	return tp;
}

date::sys_time<std::chrono::milliseconds> sscanf_parse8601( std::string const &ts ) {
	std::istringstream in{ts};
	date::sys_time<std::chrono::milliseconds> tp;
	int yr = 0;
	int mo = 0;
	int dy = 0;
	int hr = 0;
	int mi = 0;
	int sc = 0;
	int ms = 0;

	if( sscanf( ts.c_str( ), "%d-%d-%dT%d:%d:%d.%dZ", &yr, &mo, &dy, &hr, &mi, &sc, &ms ) != 7 ) {
		std::cerr << "Unknown timestamp format: " << ts << '\n';
		throw date::invalid_iso8601_timestamp{};
	}

	std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> result{
	  date::sys_days{date::year_month_day{date::year{yr}, date::month( mo ), date::day( dy )}} + std::chrono::hours{hr} +
	  std::chrono::minutes{mi} + std::chrono::seconds{sc} + std::chrono::milliseconds{ms}};

	return result;
}

int main( int argc, char **argv ) {
	auto const bench_iso8601_parser = []( std::vector<std::string> const &timestamps ) {
		uintmax_t result{0};
		for( auto const &ts : timestamps ) {
			result += date::parse_iso8601_timestamp( ts ).time_since_epoch( ).count( );
		}
		return result;
	};

	auto const bench_iso8601_parser2 = []( std::vector<std::string> const &timestamps ) {
		uintmax_t result{0};
		for( auto const &ts : timestamps ) {
			result += parse8601( ts ).time_since_epoch( ).count( );
		}
		return result;
	};

	auto const bench_iso8601_sscanf_parser = []( std::vector<std::string> const &timestamps ) {
		uintmax_t result{0};
		for( auto const &ts : timestamps ) {
			result += sscanf_parse8601( ts ).time_since_epoch( ).count( );
		}
		return result;
	};

	auto const bench_javascript_parser = []( std::vector<std::string> const &timestamps ) {
		uintmax_t result{0};
		for( auto const &ts : timestamps ) {
			result += date::parse_javascript_timestamp( ts ).time_since_epoch( ).count( );
		}
		return result;
	};

	assert( argc > 1 );
	{
		std::cout << "Using Timestamp File: " << argv[1] << '\n';
		std::ifstream infile{argv[1]};
		std::vector<std::string> timestamps{};

		for( std::string line; getline( infile, line ); ) {
			timestamps.push_back( line );
		}
		std::cout << "Testing with " << timestamps.size( ) << " timestamps\n";
		for( auto const &ts : timestamps ) {
			auto const r1 = date::parse_iso8601_timestamp( ts );
			auto const r2 = parse8601( ts );
			if( r1.time_since_epoch( ).count( ) != r2.time_since_epoch( ).count( ) ) {
				std::cout << "Difference while parsing " << ts << '\n';
				using namespace date;
				using namespace std::chrono;
				std::cout << "r1: " << r1 << '\n';
				std::cout << "r2: " << r2 << '\n';
				exit( EXIT_FAILURE );
			}
		}

		auto const r1 = daw::bench_test2( "parse_iso8601_timestamp", bench_iso8601_parser, timestamps.size( ), timestamps );
		auto const r2 = daw::bench_test2( "date_parse", bench_iso8601_parser2, timestamps.size( ), timestamps );
		assert( r1.get( ) == r2.get( ) );
	}
	if( argc <= 2 ) {
		return EXIT_SUCCESS;
	}
	{
		std::cout << "Using Javascript Timestamp File: " << argv[2] << '\n';
		std::ifstream infile{argv[2]};
		std::vector<std::string> timestamps{};

		for( std::string line; getline( infile, line ); ) {
			timestamps.push_back( line );
		}

		std::cout << "Testing with " << timestamps.size( ) << " timestamps\n";
		for( auto const &ts : timestamps ) {
			auto const r1 = date::parse_javascript_timestamp( ts );
			auto const r2 = parse8601( ts );
			auto const r3 = sscanf_parse8601( ts );
			if( r1.time_since_epoch( ).count( ) != r2.time_since_epoch( ).count( ) ) {
				std::cout << "Difference while parsing " << ts << '\n';
				using namespace date;
				using namespace std::chrono;
				std::cout << "r1: " << r1 << '\n';
				std::cout << "r2: " << r2 << '\n';
				exit( EXIT_FAILURE );
			}
		}

		auto const r1 =
		  daw::bench_test2( "parse_javascript_timestamp", bench_javascript_parser, timestamps.size( ), timestamps );
		auto const r2 = daw::bench_test2( "date_parse", bench_iso8601_parser2, timestamps.size( ), timestamps );
		auto const r3 = daw::bench_test2( "sscanf", bench_iso8601_sscanf_parser, timestamps.size( ), timestamps );
		assert( r1.get( ) == r2.get( ) );
		assert( r2.get( ) == r3.get( ) );
	}
	return EXIT_SUCCESS;
}
