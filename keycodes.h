/**
 * Definition of all required constants and 
 * scan codes table.
 */


#define USART_BAUDRATE 1200
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1 )



/* SUN keyboard commands */
#define SKBDCMD_RESET       0x01
#define SKBDCMD_BELLON      0x02
#define SKBDCMD_BELLOFF     0x03
#define SKBDCMD_SETLED      0x0e


/* USB equivalents. order of bits are different from SUN's definition. */
#define USB_LED_NLOCK          	0x01   /* Num-locgk */
#define USB_LED_CLOCK          	0x02   /* Caps-lock */
#define USB_LED_SCRLCK        	0x04   /* Scroll-lock */
#define USB_LED_CMPOSE        	0x08   /* Compose */

/* Special state characters */
#define SKBD_RESET          0xff
#define SKBD_LYOUT          0xfe
#define SKBD_ALLUP          0x7f

/* Special Keys */
#define SKBD_HELP	 0x76
#define SKBD_STOP	 0x01
#define SKBD_AGAIN	 0x03
#define SKBD_PROPS	 0x19
#define SKBD_UNDO	 0x1a
#define SKBD_FRONT	 0x31
#define SKBD_COPY	 0x33
#define SKBD_OPEN	 0x48
#define SKBD_PASTE	 0x49
#define SKBD_FIND	 0x5f
#define SKBD_CUT	 0x61
#define SKBD_POWER	 0x30


#define KEY_COMPOSE  0x43

enum HID_ConsumerCodes
{
    CKEY_Mute       = 0xE2,
    CKEY_VolumeUp   = 0xE9,
    CKEY_VolumeDown = 0xEA,
};


PROGMEM const char usbHidReportDescriptor[65] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    //    0x85, 0x01,                    //   REPORT_ID (1)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard) //10
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1) //20
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs) // 30
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)

    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana) // 40
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs)
    0x95, 0x06,                    //   REPORT_COUNT (6) //50
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated)) // 61
    0x2a, 0xff, 0x00,              //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION // 66
};


// standard v.down = 129
// standard v.up = 128
// replaced with f17 and f18

PROGMEM const uint8_t  sunkeycodes[]= {
/*?       stop    v.down  again   v.up    f1      f2      f10  */
  0,      0x78,   108,    0x79,   109,    58,     59,     67,	/* 0x00-0x07 */

/*f3      f11     f4      f12     f5      AltGr   f6      NoKey*/
  60,     68,     61,     69,     62,     230,    63,     0,	/* 0x08-0x0f */

/**/
  64,     65,     66,     226,    82,     72,     70,     71,	/* 0x10-0x17 */
  80,     0x76,   0x7A,   81,     79,     41,     30,     31,	/* 0x18-0x1f */
  32,     33,     34,     35,     36,     37,     38,     39,	/* 0x20-0x27 */
  
/*                                        mute                 */
  45,     46,     53,     42,     73,     0x6b,   84,     85,	/* 0x28-0x2f */

/*power                                                        */
  0x6e,   0x77,   99,     0x7C,   74,     43,     20,     26,	/* 0x30-0x37 */
  8,      21,     23,     28,     24,     12,     18,     19,	/* 0x38-0x3f */

/*                        compose                              */
  47,     48,     76,     101,    95,     96,     97,     86,	/* 0x40-0x47 */
  0x74,   0x7D,   77,     0,      224,    4,      22,     7,	/* 0x48-0x4f */
  9,      10,     11,     13,     14,     15,     51,     52,	/* 0x50-0x57 */
  49,     40,     88,     92,     93,     94,     98,     0x7E,	/* 0x58-0x5f */
  75,     0x7B,   83,     225,    29,     27,     6,      25,	/* 0x60-0x67 */
  5,      17,     16,     54,     55,     56,     229,    0,	/* 0x68-0x6f */

/*                                                help         */
  89,     90,     91,     0,      0,      0,      0x3A,   57,	/* 0x70-0x77 */
  227,    44,     231,    78,     0,      87,     0,      0 	/* 0x78-0x7f */
};

