//
// totem control
//
// Copyright Tarim 2012,2013
// 
// Totems is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// Totems is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Totems.  If not, see <http://www.gnu.org/licenses/>.

#include "Streaming.h"
#include "VirtualWire.h"
#include "totem.h"

// output debugging information
#define DEBUG

#define VERSIONMAJOR 0
#define VERSIONMINOR 21

//#define TX
#define RX

//
// time stuff
//
typedef uint32_t clock_t;
const clock_t INFINITE = (clock_t)(-1);
const clock_t HALFTIME = (INFINITE >> 2) + 1;
const clock_t SEC = 1000;
const clock_t MINUTE = SEC * 60;
const clock_t HOUR = MINUTE * 60;
const clock_t DAY = HOUR * 24;
const clock_t WEEK = DAY * 7;

//
// general stuff
//
#define NULL 0
#define length(x) ( sizeof(x) / sizeof(*(x)) )
#define incmod( val, modulo ) ((val) == (modulo)-1 ? 0 : (val)+1)
#define decmod( val, modulo ) ((val) == 0 ? (modulo)-1 : (val)-1)
#define quote_( x ) #x
#define quote( x ) quote_( x )

#define BAUD 9600
const byte LED = 13;

#ifdef DEBUG
    #define assert(assertion, label) typedef int label[(assertion) ? 0 : -1]
#else
    #define assert(assertion, label)
#endif

#ifdef DEBUG
extern unsigned int __data_start;
extern unsigned int __data_end;
extern unsigned int __bss_start;
extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

int freeMemory() {
int free_memory;

    if((int)__brkval == 0)
	free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else
	free_memory = ((int)&free_memory) - ((int)__brkval);
    return free_memory;
};
#endif

//
// waiter: returns true and resets if more than time has passed
//
class waiter {
public:
    clock_t last;

    bool wait( const clock_t time ) {
	clock_t now = millis();
	if( now - last >= time ) {
	    last = now;
	    return true;
	}
	return false;
    };
};

//
// syncer: returns true if different state to last time
//
typedef int sync_t;
class syncer {
public:
    sync_t modulos[4];
    const static clock_t offset = 0;

    byte state() {
	sync_t modulo;
	byte j;

        modulo = (millis() - offset + HALFTIME) % modulos[length(modulos)-1];
	for( j = 0; modulo > modulos[j]; ++j ) ;

	return j;
    };
};


#define lookup( val, array ) lookup_( val, array, length(array) )
int lookup_( const byte val, const byte array[], const byte len ) {
    unsigned int j;
    for( j = 0; j < len; ++j ) {
	if( val == array[j] ) {
	    return j;
	}
    }
    return -1;
};

const byte peltierPin = 7;
const byte vibePin = 5;
const byte commands[] =
    { CRED, CGREEN, CBLUE, CTONE, CVIBE, CTEMP, CFAST, CSLOW };
enum flags_e
    { BRED, BGREEN, BBLUE, BTONE, BVIBE, BTEMP, BFAST, BSLOW };
const byte pins[] =
    {   99,      9,    10,    11, vibePin, peltierPin };
//    {   13,      9,    10,    11, vibePin, peltierPin };
const byte pinValues[] =
    {  255,    255,   255,   240,   255 };

#ifdef TX
    const byte PRX = NOT_A_PIN;
    const byte PTX = 9;
#endif
#ifdef RX
    const byte PRX = 2;
    const byte PTX = NOT_A_PIN;
#endif

const uint16_t rfbaud = 2000;

typedef uint8_t flag_t;

const clock_t PACKETDELAY = 256;

#ifdef TX
class transmitter {
public:
    flag_t flags;
    waiter timer;

    void doCommand( const token_t token, const byte value ) {
	byte flag = lookup( token, commands );
	if( flag >= 0 ) {
	    bitWrite( flags, flag, value );
	}
    };

    void poll() {
	if( !vx_tx_active() && timer.wait( PACKETDELAY ) ) {
	    vw_send( &flags, sizeof( flags ) );
	}
    };
} transmit;

enum state_e { HEADERS, QUERY, CONTENT };

class server: public parser_c {
public:
    enum state_e state;
    value_t contentLength;

    void endCommands() {
	poll( '&' );
	state = HEADERS;
	contentLength = 0;
    };
    
    void HTTPResponse() {
	Serial << "HTTP/1.0 204 Ok " << _BIN( transmit.flags ) << "\r\n\r\n";
    };

    void fullPoll( const int ch ) {
	if( ch > 0 ) {
       	    poll( ch );

            if( state == CONTENT && ch >= ' ' && --contentLength == 0 ) {
                endCommands();
		HTTPResponse();
	    }

            else if( state == QUERY && ch == ' ' ) {
                endCommands();
            }
	}
    };
    
    void doCommand();
};

//
//  read commands from port
//
void server::doCommand() {
    switch( ch ) {
    case '\n':
	if( state != HEADERS ) {
	    // old connection must have been interrupted
	    state = HEADERS;
	    contentLength = 0;

	} else if( token == CCONTENTLENGTH && len == CCONTENTLENGTHlen ) {
	    // Content-Length: 99
	    contentLength = value;

	} else if( token == CEMPTY && len == CEMPTYlen ) {
	    // \r\n\r\n
	    if( contentLength ) {
		state = CONTENT;
	    } else {
		HTTPResponse();
	    }

	} // else ignore other commands

	break;

    case '?':
	if( state == HEADERS && token == CGET && len == CGETlen ) {
       	    // GET /?
	    state = QUERY;
	}
	break;

    case '&':
	if( state != HEADERS ) {
	    transmit.doCommand( token, value );
	}
        break;
    };

};

//server<token_t> Server;
server Server;
#endif



#ifdef RX

enum peltier_e { IDLE, ACTIVE, RECOVER };
const clock_t ACTIVETIME = 6 * SEC;
const clock_t RECOVERTIME = 15 * SEC;

class peltier_c {
public:
    peltier_e state;
    waiter timer;
    bool monitor;
    
    void setState( const peltier_e s, const bool out ) {
        digitalWrite( LED, out & monitor );
        digitalWrite( peltierPin, out );
        state = s;
    };

    void poll( const bool on, const bool m ) {
        monitor = m;
        switch( state ) {
        case IDLE:
            if( on ) {
                timer.wait( 0 );
                setState( ACTIVE, HIGH );
            }
            break;

        case ACTIVE:
            if( !on || timer.wait( ACTIVETIME ) ) {
                setState( RECOVER, LOW );
            }
            break;

        case RECOVER:
            if( timer.wait( RECOVERTIME ) ) {
                setState( IDLE, LOW );
            }
            break;
        }
    };
} peltier;


const sync_t PULSERATE = 833;
const sync_t SLOWRATE = 2000;
const sync_t FASTRATE = 333;
const sync_t BEATTIME = 50;
const sync_t SKIPTIME = 100;

class totem_c {
public:
    syncer timer;
    flag_t flags;
    bool state;

    void write() {
	int j;
	for( j = 0; j < length( pinValues ); ++j ) {
	    bool b = bitRead( flags, j ) && state;
	    analogWrite( pins[j], b ? pinValues[j] : 0 );
	}
    };

    void poll() {
	if( state == (timer.state() & 1) ) {
	    state = !state;
	    write();
	}
        peltier.poll( bitRead( flags, BTEMP ), bitRead( flags, BRED ) );
    };

    void doCommand( const flag_t f ) {
	flags = f;
	if( bitRead( flags, BSLOW ) ) {
	    timer.modulos[3] = SLOWRATE;
	} else if( bitRead( flags, BFAST ) ) {
	    timer.modulos[3] = FASTRATE;
	} else {
	    timer.modulos[3] = PULSERATE;
	}
	write();
    };
} totem = {{
    BEATTIME,
    BEATTIME + SKIPTIME,
    BEATTIME + SKIPTIME + BEATTIME,
    PULSERATE
}};

typedef
    union {
	byte raw[VW_MAX_MESSAGE_LEN];
	flag_t flags;
    }
    buffer_t;

class receiver {
public:
    buffer_t buffer;
    byte buflen;

    void poll() {
	buflen = VW_MAX_MESSAGE_LEN;
	if( vw_get_message( buffer.raw, &buflen ) ) {
	    if( buflen == sizeof( flag_t ) ) {
		totem.doCommand( buffer.flags );
	    }
	}
    };
} receive;
#endif

void setup() {
    pinMode( LED, OUTPUT );

    byte j;
    for( j = 0; j < length( pins ); ++j ) {
	pinMode( pins[j], OUTPUT );
    }

    vw_set_rx_pin( PRX );
    vw_set_tx_pin( PTX );
    vw_set_ptt_pin( NOT_A_PIN );
    vw_setup( rfbaud );	 // Bits per sec

    #ifdef RX
    	vw_rx_start();       // Start the receiver PLL running
    #endif


    #ifdef TX
	while( !Serial ) {}
        delay( 1000 );
        Serial.begin( BAUD );
        #ifdef DEBUG
	    Serial << "ver " quote(VERSIONMAJOR) "." quote(VERSIONMINOR) << endl;
	    Serial << "mem " << freeMemory() << endl;
        #endif
    #endif
};

void loop() {
    #ifdef TX
        Server.fullPoll( Serial.read() );
        transmit.poll();
    #endif

    #ifdef RX
        receive.poll();
	totem.poll();
    #endif
};
