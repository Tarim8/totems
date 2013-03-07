/*
 * parser.h
 *
 * parser_c class
 * included by token definition include file and mkparser.cc
 *
 * Copyright Tarim July 2011
 * Added token length Tarim December 2012
 *
 */


// parser class template
// gets fed characters one by one and builds up a hash value for the token
// which gets passed to the doCommand() when a token separator is read
// template <typename token_t>
class parser_c {
public:
    // parser variables
    uint16_t len;	// statement length
    value_t value;	// build up the digit value
    token_t token;	// build up the hash value
    byte ch;		// the current input value

    // digit characters
    inline bool isDigit() {
	return ch <= '9' && ch >= '0';
    };

    // characters which won't be added into the hash value
    inline bool isIgnore() {
	// PARSER_IGNORE is constant, so testing *PARSER_IGNORE allows
	// compiler to remove the call completely if PARSER_IGNORE=""
	return  ch <= PARSER_IGNOREUPTO || 
		( *PARSER_IGNORE && strchr( PARSER_IGNORE, ch ) );
    };

    // characters that end tokens. Can also be ignore characters (which
    // would terminate a token but not be added into the hash value)
    inline bool isSeparator() {
	return strchr( PARSER_SEPARATOR, ch );
    };
 
    // build up the value of the digits
    inline value_t addValue() {
	return value * 10 + ch - '0';
    };
 
    // simple DJB hash function to accumulate the hash value
    inline token_t addToken() {
	return token + (token << PARSER_HASHSHIFT) + ch;
    };

    // construct by initializing
    parser_c() {
	initialize();
    };

    // initialise for the next token
    inline void initialize() {
	token = PARSER_HASHINIT;
	value = len = 0;
    };

    // user defined command when we reach a token separator
    virtual void doCommand();

    // polling routine
    void poll( const int c ) {
	if( c >= 0 ) {
	    ch = c;

	    if( isDigit() ) {
		value = addValue();	// add to value if we have digit

	    } else if( !isIgnore() ) {
		++len;
		token = addToken();	// add to token if not ignored
	    }

	    if( isSeparator() ) {
		doCommand();		// do the command if char is separator
		initialize();		// initialise hash values
	    }
        }
    };
};

// don't declare an instance of the class
// parser_c <token_t> Parser;


// access the include file name from outside the include file
const char* const PARSER_INCLUDE_NAME = __FILE__;

