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
#include <date/chrono_io.h>
#include <date/date.h>
#include <fstream>
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
		in >> date::parse( "%FT%T%z", tp );
		if( in.fail( ) ) {
			std::cerr << "Unknown timestamp format: " << ts << '\n';
			throw invalid_iso8601_timestamp{};
		}
	}
	return tp;
}

int main( int argc, char **argv ) {
	auto const bench_iso8601_parser = []( std::vector<std::string> const &timestamps ) {
		uintmax_t result{0};
		for( auto const &ts : timestamps ) {
			result += parse_iso8601_timestamp( ts ).time_since_epoch( ).count( );
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

	auto const bench_javascript_parser = []( std::vector<std::string> const &timestamps ) {
		uintmax_t result{0};
		for( auto const &ts : timestamps ) {
			result += parse_javascript_timestamp( ts ).time_since_epoch( ).count( );
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
			auto const r1 = parse_iso8601_timestamp( ts );
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

		auto const r1 = daw::bench_test( "parse_iso8601_timestamp", bench_iso8601_parser, timestamps );
		auto const r2 = daw::bench_test( "parse_iso8601_timestamp2", bench_iso8601_parser2, timestamps );
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
			auto const r1 = parse_javascript_timestamp( ts );
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

		auto const r1 = daw::bench_test( "parse_javascript_timestamp", bench_javascript_parser, timestamps );
		auto const r2 = daw::bench_test( "parse_iso8601_timestamp2", bench_iso8601_parser2, timestamps );
		assert( r1.get( ) == r2.get( ) );
	}
	return EXIT_SUCCESS;
}

