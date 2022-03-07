#ifndef __STRING_OPS__
#define __STRING_OPS__

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

typedef std::vector<std::string>   TokenList;

inline TokenList split( std::string s, char splitter )
{
	TokenList tokens;

	unsigned offset = s.find( splitter );

	if ( offset == std::string::npos )
		return tokens;

	
	std::string token = s.substr( 0, offset );
	if ( !token.empty() )
		tokens.push_back( token );
	s = s.substr( offset + 1, s.size() - (offset+1) );
	
	while( !s.empty() )
	{
		offset = s.find( splitter );
		if (offset == std::string::npos )
		{
			tokens.push_back(s);
			return tokens;
		}
		token = s.substr( 0, offset );
		if ( !token.empty() )
			tokens.push_back( token );
		s = s.substr( offset + 1, s.size() - (offset+1));
	}
	
	return tokens;
}

template <class T>
std::string to_string( T& value, std::ios_base& (*converter)( std::ios_base& ) )
{
	std::stringstream ss;
	ss << converter << value;
	return ss.str();
}

template <class T>
bool from_string( T& value, const std::string& s, std::ios_base& (*converter)( std::ios_base& ) )
{
	std::istringstream iss( s );
	return !(iss >> converter >> value).fail();
}

#endif // string_ops.hxx
