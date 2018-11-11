/*
 *  THEC64 Mini
 *  Copyright (C) 2017 Chris Smith
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/time.h>

#include "flashlight_mapfile.h"
#include "tsf_joystick.h"

static const char *DeviceStem = "/dev/input/"; // MUST end in a '/'

//#define JSDEBUG
#ifdef JSDEBUG
#   define printd(...) printf(__VA_ARGS__)
#else
#   define printd(...) {;} //(0)
#endif

#define BLOCKING

typedef struct {
    tsf_input_type_t  type;
    int               name_len;
    char             *name;
} tsf_input_name_t;

static tsf_input_name_t input_name_map[] = {
    { INPUT_X,            1, "x"},
    { INPUT_Y,            1, "y"},
    { INPUT_A,            1, "a"},
    { INPUT_B,            1, "b"},
    { INPUT_BACK,         4, "back"},
    { INPUT_GUIDE,        5, "guide"},
    { INPUT_START,        5, "start"},
    { INPUT_DIG_LEFT,     6, "dpleft"},
    { INPUT_DIG_RIGHT,    7, "dpright"},
    { INPUT_DIG_UP,       4, "dpup"},
    { INPUT_DIG_DOWN,     6, "dpdown"},
    { INPUT_SHLD_LEFT,   12, "leftshoulder"},
    { INPUT_TRIG_LEFT,   11, "lefttrigger"},
    { INPUT_SHLD_RIGHT,  13, "rightshoulder"},
    { INPUT_TRIG_RIGHT,  12, "righttrigger"},
    { INPUT_STICK_LEFT,   9, "leftstick"},
    { INPUT_STICK_RIGHT, 10, "rightstick"},
    { INPUT_AXIS_LEFT_X,  5, "leftx"},
    { INPUT_AXIS_LEFT_Y,  5, "lefty"},
    { INPUT_AXIS_RIGHT_X, 6, "rightx"},
    { INPUT_AXIS_RIGHT_Y, 6, "righty"},

    // These are only for id_to_string() conversions, not SDL gamecontrollerdb.txt mapping 
    { INPUT_AXIS_LEFT_X_LEFT,   0, "leftx left"},
    { INPUT_AXIS_LEFT_X_RIGHT,  0, "leftx right"},
    { INPUT_AXIS_LEFT_Y_UP,     0, "lefty up"},
    { INPUT_AXIS_LEFT_Y_DOWN,   0, "lefty down"},
    { INPUT_AXIS_RIGHT_X_LEFT,  0, "rightx left"},
    { INPUT_AXIS_RIGHT_X_RIGHT, 0, "rightx right"},
    { INPUT_AXIS_RIGHT_Y_UP,    0, "righty up"},
    { INPUT_AXIS_RIGHT_Y_DOWN,  0, "righty down"},
};

static int   tsf_enumerate_buttons( tsf_joystick_device_t *js );
static int   tsf_enumerate_axis( tsf_joystick_device_t *js );
static int   tsf_enumerate_hats( tsf_joystick_device_t *js );

// ----------------------------------------------------------------------------
static void 
ev_to_guid(struct libevdev *dev, uint16_t * guid) {
    guid[0] = (uint16_t)(libevdev_get_id_bustype(dev));
    guid[1] = 0;
    guid[2] = (uint16_t)(libevdev_get_id_vendor(dev));
    guid[3] = 0;
    guid[4] = (uint16_t)(libevdev_get_id_product(dev));
    guid[5] = 0;
    guid[6] = (uint16_t)(libevdev_get_id_version(dev));
    guid[7] = 0;
}

// ----------------------------------------------------------------------------
// This returns the active count of joysticks.
// It is the total currently registered minus the number of queued de-registrations
int
tsf_active_joystick_count( tsf_joystick_t *j )
{
    return j->js_count - j->deregister_queue_count;
}

// ----------------------------------------------------------------------------
static void
queue_deregister_joystick_map( tsf_joystick_t *j, int js_id )
{
    if( j->deregister_queue_count >= DEREGISTER_QUEUE_MAX ) return;

    j->deregister_queue[ j->deregister_queue_count++ ] = js_id;
}

// ----------------------------------------------------------------------------
// Removes a joystick ID from the id to port map, and shuffles higher port
// number mappings down so the port sequence is contiguous.
static void
deregister_joystick_map( tsf_joystick_t *j, int js_id )
{
    if( js_id >= MAX_JOYSTICKS ) return;

    int old_port_number = j->joystick_map[ js_id ];

    printd("Deregister JS ID %d. Currently mapped to port %d\n", js_id, old_port_number );
    j->joystick_map[ js_id ] = 0;

    int i = 0;
    // Shuffle port allocation down.
    for(i = 0; i < j->js_count; i++ ) {

#ifdef JSDEBUG
    printf("ID %d is mapped to %d\n", i,  j->joystick_map[ i ] );
    if( j->joystick_map[ i ] > old_port_number ) printf(".. shift down to port %d\n", j->joystick_map[ i ]-1);
#endif

        if( j->joystick_map[ i ] > old_port_number ) j->joystick_map[ i ]--;
    }

    j->js_count--;
}

// ----------------------------------------------------------------------------
// Registers the joystick (not keyboard or switch) and assigns it an id.
// It maps this id to a port number between 1 and MAX_JOYSTICKS (contiguous).
// Returns the ID of the registered joystick
static int
register_joystick_map( tsf_joystick_t *j )
{
    int ret = -1;

    int js_id = 0;
    for( js_id = 0; js_id < j->js_count; js_id++ ) {
        if( j->joystick_map[ js_id ] == 0 ) break;
    }

    if( js_id < MAX_JOYSTICKS ) {
        j->js_count++;
        j->joystick_map[ js_id ] = j->js_count;

        printd("Allocating JS ID %d to virtual port %d\n", js_id, j->js_count );

        ret = js_id;
    }
    return ret;
}

// ----------------------------------------------------------------------------
static void
guid_to_hex(uint16_t * guid, char * guidstr) {
    static const char hexchar[] = "0123456789abcdef";
    int i;
    for (i = 0; i < 8; i++) {
        unsigned char c = guid[i];

        *guidstr++ = hexchar[c >> 4];
        *guidstr++ = hexchar[c & 0x0F];

        c = guid[i] >> 8;
        *guidstr++ = hexchar[c >> 4];
        *guidstr++ = hexchar[c & 0x0F];
    }
    *guidstr = '\0';
}

// ----------------------------------------------------------------------------
//
tsf_joystick_t *
tsf_initialise_joysticks()
{
    tsf_joystick_t *j = malloc( sizeof(tsf_joystick_t) );
    if( j == NULL ) {
        return NULL;
    }
    memset( j, 0, sizeof(tsf_joystick_t) );

    j->count     = 0;
    j->js_count  = 0;
    j->sw_count  = 0;
    j->key_count = 0;

    int i;
    tsf_joystick_device_t *js;
    for( i = 0; i < MAX_DEVICES; i++ ) {

        js = &(j->joystick[i]);

        js->button_count = 0;
        js->axis_count   = 0;
        js->hat_count    = 0;
        js->remapped     = 0;
    }

    return j;
}

// ----------------------------------------------------------------------------
//
static int
connect_joystick( tsf_joystick_t *j, const char *dname, int dlen )
{
    int fd;
    int rc;
    int i;

    static char jpath[256] = {0};
    static int  jpathlen   = 0;

    if( jpathlen == 0 ) {
        jpathlen = strlen(DeviceStem);
        memcpy( jpath, DeviceStem, jpathlen );
    }

    printd("Opening event file %s%s\n", DeviceStem, dname);

    int free_slot = -1;
    // See if we are already connected
    for( i = 0; i < j->count; i++ ) {
        if( strcmp( j->joystick[i].device_path, dname ) == 0 ) {
            printd( "Already connected device %s\n", dname );
            return 0;
        }
        // Just in case a device has been removed, reuse it's slot for new device.
        if( j->joystick[i].evdev == NULL && free_slot < 0 ) free_slot = i;
    }

    if( j->count == MAX_DEVICES ) return -1;

    memcpy( jpath + jpathlen, dname, dlen + 1 ); // +1 for null

    fd = open( jpath, O_RDONLY | O_NONBLOCK);
    if( fd < 0 ) {
        //printd( "Could not open joystick %s: %d\n", jpath, errno );
        return -1;
    }

    struct libevdev *dev;
    rc = libevdev_new_from_fd( fd, &dev );
    if (rc < 0) {
        printf("Failed to init libevdev (%s)\n", strerror(-rc));
        close(fd);
        return -1;
    }

    // Query capability
    // EV_KEY && !code BTN_* &&   EV_LED  && ! EV_ABS  = Keyboard
    // EV_KEY &&  code BTN_* && ! EV_LED  && ! EV_REL  = Joystick   -- not sure about EV_REL
    jstype_t type = JOY_UNKNOWN;
    if(  libevdev_has_event_type( dev, EV_KEY ) && 
        !libevdev_has_event_type( dev, EV_ABS ) && 
         libevdev_has_event_code( dev, EV_KEY, KEY_POWER) &&
        !libevdev_has_event_code( dev, EV_KEY, KEY_A)) {

        printd("%s is a power switch etc\n", jpath );
        type = JOY_SWITCHES;
    }
    if(  libevdev_has_event_type( dev, EV_KEY ) && 
         libevdev_has_event_type( dev, EV_LED ) && 
        !libevdev_has_event_type( dev, EV_ABS ) && 
         libevdev_has_event_code( dev, EV_KEY, KEY_A)) {

        printd("%s is a keyboard\n", jpath );
        type = JOY_KEYBOARD;
    }
    if(  libevdev_has_event_type( dev, EV_KEY ) && 
        !libevdev_has_event_type( dev, EV_LED ) && 
        !libevdev_has_event_type( dev, EV_REL ) && 
        !libevdev_has_event_code( dev, EV_KEY, KEY_POWER) &&
        !libevdev_has_event_code( dev, EV_KEY, KEY_A)) {
        // TODO Should we be checking ABS aswell here?

        printd("%s is a joystick\n", jpath );
        type = JOY_JOYSTICK;
    }

    if( type == JOY_JOYSTICK && j->js_count >= MAX_JOYSTICKS ) {
        // Too many joysticks connected
        libevdev_free( dev );
        close( fd );
        return -1;
    }

    if( type == JOY_UNKNOWN ) {
#ifdef SCANNING
		// Scan device to work out what it's capabilities are
		printf("Scanning unknown device %s to see what it is capable of (just for debug)\n", jpath );
        int i;
        for (i = BTN_JOYSTICK; i < KEY_MAX; ++i) {
            if (libevdev_has_event_code(dev, EV_KEY, i)) {
                printd("%s has button: 0x%x - %s\n", jpath, i, libevdev_event_code_get_name(EV_KEY, i));
            }
        }
        for (i = BTN_MISC; i < BTN_JOYSTICK; ++i) {
            if (libevdev_has_event_code(dev, EV_KEY, i)) {
                printd("%s has button: 0x%x - %s\n", jpath, i, libevdev_event_code_get_name(EV_KEY, i));
            }
        }
        for (i = KEY_ESC; i < KEY_MICMUTE; ++i) {
            if (libevdev_has_event_code(dev, EV_KEY, i)) {
                printd("%s has button: 0x%x - %s\n", jpath, i, libevdev_event_code_get_name(EV_KEY, i));
            }
        }
#endif
        libevdev_free( dev );
        close( fd );
        return -1;
    }

    int next_count = j->count;

    if( free_slot < 0 ) {
        // We're not reusing a slot, so append to the end and increase total count
        free_slot = j->count;
        next_count++;
    }

    // Get next joystick slot
    tsf_joystick_device_t *js = &( j->joystick[ free_slot ] );
    js->evdev = dev;

    js->type = type;

    if( js->type == JOY_KEYBOARD ) { js->id = j->key_count++; } // TODO Use fd?
    if( js->type == JOY_SWITCHES ) { js->id = j->sw_count++;  } // TODO Use fd?
    if( js->type == JOY_JOYSTICK ) {
        int js_id = register_joystick_map( j );
        if( js_id < 0 ) return -1;
        js->id = js_id;
    }

    strncpy( js->device_path, dname, sizeof(js->device_path) - 1 );

    printd("Input device name: \"%s\"\n", libevdev_get_name(js->evdev));
    printd("Input device ID: bus %#x vendor %#x product %#x\n", // unique %s\n",
            libevdev_get_id_bustype(js->evdev),
            libevdev_get_id_vendor(js->evdev),
            libevdev_get_id_product(js->evdev) );
            //libevdev_get_uniq);

    ev_to_guid (js->evdev, js->guid);
    guid_to_hex(js->guid,  js->guidhex);

    printd("Input device GUID %s\n",  js->guidhex );
    printd("Vendor: %x\n",            js->guid[2] );
    printd("Device: %x\n",            js->guid[4] );
    printd("Type  : %d\n",            js->type);
    printd("ID    : %d\n",            js->id );
    printd("Slot  : %d\n",            free_slot );

    j->pollfds[ free_slot ].fd = fd; //libevdev_get_fd( js->evdev );
    j->pollfds[ free_slot ].events = POLLIN | POLLHUP;
    j->pollfds[ free_slot ].revents = 0;

    j->count = next_count;

    printd( "Connected with fd %d\n", fd );

    return 1;
}

// ----------------------------------------------------------------------------
// Returns > 0 if new joystick attached or < 0 if there is an error
int
tsf_scan_for_joystick( tsf_joystick_t * j )
{
    int fd;
    int rc = 1;
    DIR *dir;
    int flen;
    int found = 0;

    // First deregister and remap any previously removed joysticks.
    // This is done to defer remaping virtual port allocation while the allocation
    // may be being used.
    int i;
    for(i = 0; i < j->deregister_queue_count; i++ ) {
        deregister_joystick_map( j, j->deregister_queue[ i ] );
    }
    j->deregister_queue_count = 0;

    // Now look for new joysticks
    dir = opendir( DeviceStem );
    if( !dir ) {
        printf("Error: could not open directory %s: %s\n", DeviceStem, strerror(errno) );
        return -1;
    }

    const char *fname;
    struct dirent *dent;
    while ((dent = readdir (dir))) {
        fname = dent->d_name;
        flen = strlen(fname);

        // Open device (/dev/input/event*), and query capabilities.
        if( flen > 5 && strncmp( fname, "event", 5 ) == 0 ) {
            if( (rc = connect_joystick( j, fname, flen )) < 0 ){
                printd("Error connecting joystick %s\n", fname );
            } else if( rc > 0 && found == 0 ) {
                found = 1;
            }
        }
    }
    if( closedir(dir) ) {
        printf("Error: could not close directory %s: %s\n", DeviceStem, strerror(errno) );
        return -1;
    }

    return found;
}

// ----------------------------------------------------------------------------
//
int
tsf_enumerate_keys( tsf_joystick_device_t *js )
{
    int i,x;

    js->button_count = 0;
    for (i = KEY_ESC; i < KEY_MICMUTE + 1; ++i) {
        if (libevdev_has_event_code(js->evdev, EV_KEY, i)) {
            printd("%d - Keyboard has key: 0x%x - %s\n", js->button_count, i,
                    libevdev_event_code_get_name(EV_KEY, i));

            x = i - KEY_ESC;
            js->button[ x ]                      = i; //js->button_count;
            js->button_index[ js->button_count ] = x;

            js->button_count++;
        }
    }
    return js->button_count;
}

// ----------------------------------------------------------------------------
//
static int
tsf_enumerate_buttons( tsf_joystick_device_t *js )
{
    int i,x;

    js->button_count = 0;
    for (i = BTN_JOYSTICK; i < KEY_MAX; ++i) {
        if (libevdev_has_event_code(js->evdev, EV_KEY, i)) {
            printd("%d - Joystick has button: 0x%x - %s\n", js->button_count, i,
                    libevdev_event_code_get_name(EV_KEY, i));

            x = i - BTN_MISC;
            js->button[ x ]                      = js->button_count;
            js->button_index[ js->button_count ] = x;

            js->button_count++;
        }
    }
    for (i = BTN_MISC; i < BTN_JOYSTICK; ++i) {
        if (libevdev_has_event_code(js->evdev, EV_KEY, i)) {
            printd("%d - Joystick has button: 0x%x - %s\n", js->button_count, i,
                    libevdev_event_code_get_name(EV_KEY, i));

            x = i - BTN_MISC;
            js->button[ x ]                      = js->button_count;
            js->button_index[ js->button_count ] = x;

            js->button_count++;
        }
    }
    return js->button_count;
}

// ----------------------------------------------------------------------------
//
static int
tsf_enumerate_axis( tsf_joystick_device_t *js )
{
    int i;

    js->axis_count = 0;
    for (i = 0; i < ABS_MAX; ++i) {
        /* Skip hats */
        if (i == ABS_HAT0X) {
            i = ABS_HAT3Y;
            continue;
        }
        if (libevdev_has_event_code(js->evdev, EV_ABS, i)) {
            const struct input_absinfo *absinfo = libevdev_get_abs_info(js->evdev, i);
            printd("Joystick has absolute axis: 0x%.2x - %s\n", i, libevdev_event_code_get_name(EV_ABS, i));
            printd("Values = { %d, %d, %d, %d, %d }\n",
                   absinfo->value, absinfo->minimum, absinfo->maximum,
                   absinfo->fuzz, absinfo->flat);

            int threshold = (absinfo->maximum - absinfo->minimum) / 3;

            js->axis_min[i] = absinfo->minimum + threshold;
            js->axis_max[i] = absinfo->maximum - threshold;
            js->axis[i]     = js->axis_count;

            js->axis_index[ js->axis_count ] = i;

            js->axis_count++;
        }
    }
    return js->axis_count;
}

// ----------------------------------------------------------------------------
//
static int
tsf_enumerate_hats( tsf_joystick_device_t *js )
{
    int i,x,threshold;

    js->hat_count = 0;
    for (i = ABS_HAT0X; i <= ABS_HAT3Y; i += 2) {
        if (libevdev_has_event_code(js->evdev, EV_ABS, i) || libevdev_has_event_code(js->evdev, EV_ABS, i+1)) {
            const struct input_absinfo *absinfo = libevdev_get_abs_info(js->evdev, i);
            if (absinfo == NULL) continue;

            x = (i - ABS_HAT0X) / 2;

            printd("Joystick has hat %d\n", x);
            printd("Values = { %d, %d, %d, %d, %d }\n",
                   absinfo->value, absinfo->minimum, absinfo->maximum,
                   absinfo->fuzz, absinfo->flat);


            js->hat_min[x] = absinfo->minimum + threshold;
            js->hat_max[x] = absinfo->maximum - threshold;
            js->hat[x]     = js->hat_count;

            js->hat_index[ js->hat_count ] = x;

            js->hat_count++;
        }
    }

    return js->hat_count;
}

// ----------------------------------------------------------------------------
//
int
tsf_enumerate_joysticks( tsf_joystick_t * j ) 
{
    int i;
    tsf_joystick_device_t *js;

    for( i = 0; i < j->count; i++ ) {
        js = &(j->joystick[i]);

        if( !js->enumerated && js->evdev) {
            if( js->type == JOY_JOYSTICK ) {
                tsf_enumerate_buttons( js );
                tsf_enumerate_axis( js );
                tsf_enumerate_hats( js );
            } else {
                tsf_enumerate_keys( js );
            }
            js->enumerated = 1;
        }
    }
}

// ----------------------------------------------------------------------------
//
static tsf_input_type_t 
convert_sdl_map_id( char *p, int len )
{
    int i;
    for( i = 0; i < sizeof(input_name_map) / sizeof(input_name_map[0]); i++ ) {
        if( len > 0 && len == input_name_map[i].name_len && 
            strncmp( input_name_map[i].name, p, len ) == 0 ) {
            //printf( "Mapped %*.*s to 0x%x\n", len, len, p, input_name_map[i].type );
            return input_name_map[i].type;
        }
    }
    return INPUT_NONE;
}

// ----------------------------------------------------------------------------
//
char *
tsf_joystick_id_to_string( tsf_input_type_t type )
{
    int i;
    for( i = 0; i < sizeof(input_name_map) / sizeof(input_name_map[0]); i++ ) {
        if( input_name_map[i].type == type ) return input_name_map[i].name;
    }
    return "";
}
// ----------------------------------------------------------------------------
//
static int
reconfigure_mapping( tsf_joystick_device_t *js, char *mapping )
{
    int ix;
    char *s, *c;
    tsf_input_type_t type;

    char *p = mapping;

    char *e = p;
    while( *e != '\n' && *e != '\0' ) e++;

    while( p < e ) {
        c = p; while( c < e && *c != ':' ) c++; // Find colon
        s = c; while( s < e && *s != ',' ) s++; // Find comma

        if( c == p || s == p  || c == s ) return 0;  // We're done

        c++;

        if( *c == 'b' ) {
            // Button
            ix = atoi( c + 1 );
            if( ix < js->button_count ) {
                type = convert_sdl_map_id( p , c - p - 1 );
                js->button[ js->button_index[ ix ] ] = type;
            } else {
                // error
                printf( "error, got button ix of %d. Max is %d\n", ix, js->button_count );
            }
        }
        if( *c == 'a' ) {
            // Axis
            ix = atoi( c + 1 );
            if( ix < js->axis_count ) {
                type = convert_sdl_map_id( p, c - p - 1 );
                js->axis[ js->axis_index[ ix ] ] = type;
            } else {
                // Error!
                printf( "Error: Axis %d out of range of max. %d\n", ix, js->axis_count );
            }
        }
        // TODO Hats
        p = s + 1;
    }
}

// ----------------------------------------------------------------------------
//
static char *
find_entry_by_guid( tsf_joystick_device_t *js, char *map, int len )
{
    char *p = map;
    char *e = p + len;

    while( p < e && (e - p) > 32 ) {

        // TODO Test this with \r\n line endings
        //
        while( p < e && *p == ' ' ) p++;

        if( *p != '#' && *p != '\n' ) {
            if( p < e && strncmp( p, js->guidhex, 32 ) == 0 ) {
                p += 33;
                while( p < e && *p != ',' ) p++; // Skip name
                p++;
                return (p < e) ? p : NULL;
            }
        }
        while( p < e && *p != '\n' ) p++;
        p++;
    }
    return NULL;
}

// ----------------------------------------------------------------------------
//
int
tsf_reconfigure_from_sdl_map( tsf_joystick_t *j, const char *fname )
{
    int fd;
    static void *mapdb = NULL;
    static int   len = 0;
    int   i;

    printd( "tsf_reconfigure_from_sdl_map (count %d)\n",j->count );

    if( !mapdb ) {
        mapdb = flashlight_mapfile( fname, &len );
        if( mapdb == NULL ) {
            fprintf( stderr, "Could not open joystick map %s\n", fname );
            return 0;
        }
    }

    tsf_joystick_device_t *js;
    char *m;

    for( i = 0; i < j->count; i++ ) {
        js = &(j->joystick[i]);

        if( js->type == JOY_JOYSTICK && js->remapped == 0 ) {
            printd( "- remapping... %s\n",  js->guidhex );

            m = find_entry_by_guid( js, (char*)mapdb, len );
            if( m != NULL ) {
                printd( "Matched GUID [%s] in %s\n", js->guidhex, fname );
                reconfigure_mapping( js, m );
                js->remapped = 1;
            }
        }
    }

    //unmapfile( mapdb, len );

    return 1;
}

// ----------------------------------------------------------------------------
//
static int
process_switch_input( tsf_joystick_device_t *js, jsEventHandler callback )
{
    struct input_event ev;
    int rc, val;

    rc = libevdev_next_event( js->evdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
    if( rc != LIBEVDEV_READ_STATUS_SUCCESS ) return rc;

    switch (ev.type) {
        case EV_KEY:
            //printf("Button %08x (Value? %d)\n", js->button[ev.code - BTN_MISC], ev.value);
            if( ev.value > 0 ) {
                callback( js, js->button[ ev.code - KEY_ESC ], 255 ); // ON
            } else {
                callback( js, js->button[ ev.code - KEY_ESC ], 0 ); // Off
            }
            break;
        default:
            break;
    }

    return 0;
}

// ----------------------------------------------------------------------------
static int
process_keyboard_input( tsf_joystick_device_t *js, jsEventHandler callback )
{
    struct input_event ev;
    int rc, val;

    rc = libevdev_next_event( js->evdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
    if( rc != LIBEVDEV_READ_STATUS_SUCCESS ) return rc;

    switch (ev.type) {
        case EV_KEY:
            printd("Key (code %d) 0x%08x (Value? %d)\n", ev.code, js->button[ev.code - KEY_ESC], ev.value);
            if( ev.value == 1 ) {
                callback( js, js->button[ ev.code - KEY_ESC ], 255 ); // ON
            } else if( ev.value == 0 ) {
                callback( js, js->button[ ev.code - KEY_ESC ], 0 ); // Off
            }
            // else if( ev.value == 2 ) { }  // Autorepeat
            break;
        default:
            break;
    }
    return 0;
}

// ----------------------------------------------------------------------------
static int
process_joystick_input( tsf_joystick_device_t *js, jsEventHandler callback )
{
    struct input_event ev;
    tsf_input_type_t ind;
    int rc, val;

    rc = libevdev_next_event( js->evdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
    if( rc != LIBEVDEV_READ_STATUS_SUCCESS ) return rc;

    switch (ev.type) {
        case EV_KEY:
            if (ev.code >= BTN_MISC) {
                ind = js->button[ ev.code - BTN_MISC ];
                //printf("Button %08x (Value? %d)\n", ind, ev.value);

                if( ! (ind & INPUT_CTRL_MASK) ) break; // Skip if unmapped

                if( ev.value > 0 ) {
                    callback( js, js->button[ ev.code - BTN_MISC ], 255 ); // ON
                } else {
                    callback( js, js->button[ ev.code - BTN_MISC ], 0 ); // Off
                }
            }
            break;
        case EV_ABS:
            val = ev.value;
            switch (ev.code) {
                case ABS_HAT0X:
                case ABS_HAT0Y:
                case ABS_HAT1X:
                case ABS_HAT1Y:
                case ABS_HAT2X:
                case ABS_HAT2Y:
                case ABS_HAT3X:
                case ABS_HAT3Y:
                    ev.code -= ABS_HAT0X;
                    //printf("Hat %d Axis %d Value %d\n", ev.code / 2, ev.code % 2, ev.value);

                    val -= js->hat_min[ev.code];
                    if( val >= 0 && val < 8 ) {
                        // value passed to callback is then processed into a direction bit or possibly a simulated keypress
                        // int hat_or[] = { 1, 9, 8, 10, 2, 6, 4, 5, };
                        // OR value = hat_or[val]
                        //callback->( js->hat[ ev.code / 2 ], val ); // let callback map value to hat_or
                    }
                    break;
                default:
                    {
                        ind = js->axis[ev.code];

                        //printf("Axis %08x: value %d  (ctrl masked: 0x%08x)\n", ind, ev.value, ind & INPUT_CTRL_MASK );
                        if( ! (ind & INPUT_CTRL_MASK) ) break; // Skip if unmapped

                        if( val <= js->axis_min[ ev.code ] ){
                            // Return the tsf_input_type_t assigned to this axis direction (LEFT or UP)
                            //
                            if( ind & _INPUT_AXIS ) {
                                if( ind & _INPUT_DIR_X ) ind |= INPUT_LEFT;
                                if( ind & _INPUT_DIR_Y ) ind |= INPUT_UP;
                            }
                            callback( js, ind, 255 ); 
                        } else if( val >= js->axis_max[ ev.code ] ) {
                            // Return the tsf_input_type_t assigned to this axis direction (RIGHT or Down)

                            if( ind & _INPUT_AXIS ) {
                                if( ind & _INPUT_DIR_X ) ind |= INPUT_RIGHT;
                                if( ind & _INPUT_DIR_Y ) ind |= INPUT_DOWN;
                            }
                            callback( js, ind, 255 ); 
                        } else {
                            callback( js, ind, 0 ); 
                        }
                    }
                    break;
            }
            break;
        case EV_REL:
            switch (ev.code) {
                case REL_X:
                case REL_Y:
                    ev.code -= REL_X;
                    //printf("Ball %d Axis %d Value %d\n", ev.code / 2, ev.code % 2, ev.value);
                    break;
                default:
                    break;
            }
            break;
    }
    return 0;
}

// ----------------------------------------------------------------------------
//
int
tsf_read_joystick( tsf_joystick_t *j, jsEventHandler callback, int timeout )
{
    fd_set rset;
    tsf_joystick_device_t *js;

    if( j == NULL ) return 0;

    if( poll( j->pollfds, j->count, timeout ) < 0 ) { 
        return 0;
    }

    int i,rc;
    for( i = 0; i < j->count; i++ ) {

        rc = 0;

        if( j->pollfds[i].revents & POLLIN ) {
            js = &j->joystick[i];
            if( !js->evdev ) continue; // This device has been removed. We should get a POLLIN for this device, but just in case.

            switch( js->type ) {
                case JOY_JOYSTICK:
                    do {
                        rc = process_joystick_input( js, callback );
                    } while ( rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == LIBEVDEV_READ_STATUS_SYNC );
                    break;

                case JOY_KEYBOARD:
                    do {
                        rc = process_keyboard_input( js, callback );
                    } while ( rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == LIBEVDEV_READ_STATUS_SYNC );
                    break;

                case JOY_SWITCHES:
                    do {
                        rc = process_switch_input( js, callback );
                    } while ( rc == LIBEVDEV_READ_STATUS_SUCCESS || rc == LIBEVDEV_READ_STATUS_SYNC );
                    break;
            }

            j->pollfds[i].revents ^= POLLIN;
        }

        // Check disconnect of JS
        if( (rc < 0 && rc != -EAGAIN) || j->pollfds[i].revents & POLLHUP ) {
            printf( "Got HUP/Fatal error on joystick fd %d\n", j->pollfds[i].fd );

            js = &j->joystick[i];

            if( js->type == JOY_JOYSTICK ) {
                // Deregiter joystick form "port", and shuffle higher port allocations down.
                // We queue the request until the next scan phase, to avaoid changing a
                // joysticks virtual port allocation while it is being used.
                queue_deregister_joystick_map( j, js->id );
            }

            if( js->type == JOY_KEYBOARD ) j->key_count--;
            if( js->type == JOY_SWITCHES ) j->sw_count--;

            libevdev_free( js->evdev );

            memset( js, 0, sizeof(tsf_joystick_device_t) );

            j->joystick[i].device_path[0] = '\0';
            close( j->pollfds[i].fd );

            j->pollfds[i].revents ^= POLLHUP;
        }
    }

    return 1;
}

