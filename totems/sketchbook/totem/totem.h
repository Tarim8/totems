// Automatically generated tokens

// arduino commands


// Headers
typedef uint8_t token_t;
typedef uint16_t value_t;
typedef uint16_t len_t;
const token_t PARSER_HASHINIT = 0;
const uint8_t PARSER_HASHSHIFT = 1;
const char PARSER_IGNOREUPTO = '&';
const char* const PARSER_IGNORE = "";
const char* const PARSER_SEPARATOR = "\n\?&";

const token_t CEMPTY = 0;		// 
const len_t CEMPTYlen = 0;		// 
const token_t CCONTENTLENGTH = 136;		// Content-Length: 99
const len_t CCONTENTLENGTHlen = 15;		// Content-Length: 99
const token_t CGET = 126;		// GET /?
const len_t CGETlen = 5;		// GET /?

// Query string variables
//@lengthsuffix 
const token_t CACTIVATE = 210;		// activate=1&
const len_t CACTIVATElen = 9;		// activate=1&
const token_t CGROUP = 54;		// group=255&
const len_t CGROUPlen = 6;		// group=255&
const token_t CRED = 252;		// red=255&
const len_t CREDlen = 4;		// red=255&
const token_t CBLUE = 239;		// blue=255&
const len_t CBLUElen = 5;		// blue=255&
const token_t CGREEN = 146;		// green=255&
const len_t CGREENlen = 6;		// green=255&

const token_t CTONE = 179;		// tone=255&
const len_t CTONElen = 5;		// tone=255&
const token_t CVIBE = 230;		// vibrate=255&
const len_t CVIBElen = 8;		// vibrate=255&
const token_t CPULSE = 48;		// pulse=255&
const len_t CPULSElen = 6;		// pulse=255&
const token_t CTEMP = 189;		// temp=255&
const len_t CTEMPlen = 5;		// temp=255&

const token_t CSLOW = 80;		// slow=255&
const len_t CSLOWlen = 5;		// slow=255&
const token_t CFAST = 37;		// fast=255&
const len_t CFASTlen = 5;		// fast=255&

#include "parser.h"
