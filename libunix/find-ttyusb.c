// engler, cs140e: your code to find the tty-usb device on your laptop.
#include <assert.h>
#include <fcntl.h>
#include <string.h>

#include "libunix.h"

#define _SVID_SOURCE
#include <dirent.h>
static const char *ttyusb_prefixes[] = {
	"ttyUSB",	// linux
    "ttyACM",   // linux
	"cu.SLAB_USB", // mac os
    "cu.usbserial", // mac os
    // if your system uses another name, add it.
	0
};

static int filter(const struct dirent *d) {
    // scan through the prefixes, returning 1 when you find a match.
    // 0 if there is no match.
    const char **filter;

    for (filter = ttyusb_prefixes; *filter; ++filter) {
        if (strstr(d->d_name, *filter) != NULL) return 1;
    }
    return 0;
}

// find the TTY-usb device (if any) by using <scandir> to search for
// a device with a prefix given by <ttyusb_prefixes> in /dev
// returns:
//  - device name.
// error: panic's if 0 or more than 1 devices.
char *find_ttyusb(void) {
    // use <alphasort> in <scandir>
    // return a malloc'd name so doesn't corrupt.
    struct dirent **nameList;
    int n;

    n = scandir("/dev", &nameList, filter, alphasort);
    if (n == -1) {
        perror("scandir");
        exit(EXIT_FAILURE);
    }
    else {
        if (n > 1 || n == 0) {
            while (n--) {
                printf("%s\n", nameList[n]->d_name);
                free(nameList[n]);
            }
            free(nameList);
            panic("0 or more than one USB device!");
        } else {
            char *path = "/dev/";
            char *str3 = (char *) malloc(1 + strlen(path)+ strlen(nameList[0]->d_name));
            strcpy(str3, path);
            strcat(str3, nameList[0]->d_name);

            char *ret = strdupf(str3);
            free(str3);
            free(nameList[0]);
            free(nameList);
            return ret;
        }
    }
}

// return the most recently mounted ttyusb (the one
// mounted last).  use the modification time 
// returned by state.
char *find_ttyusb_last(void) {
    return find_ttyusb();
}

// return the oldest mounted ttyusb (the one mounted
// "first") --- use the modification returned by
// stat()
char *find_ttyusb_first(void) {
    return find_ttyusb();
}
