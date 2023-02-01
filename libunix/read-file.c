#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "libunix.h"

// allocate buffer, read entire file into it, return it.   
// buffer is zero padded to a multiple of 4.
//
//  - <size> = exact nbytes of file.
//  - for allocation: round up allocated size to 4-byte multiple, pad
//    buffer with 0s. 
//
// fatal error: open/read of <name> fails.
//   - make sure to check all system calls for errors.
//   - make sure to close the file descriptor (this will
//     matter for later labs).
// 
void *read_file(unsigned *size, const char *name) {
    // How: 
    //    - use stat to get the size of the file.
    //    - round up to a multiple of 4.
    //    - allocate a buffer
    //    - zero pads to a multiple of 4.
    //    - read entire file into buffer.  
    //    - make sure any padding bytes have zeros.
    //    - return it.
    struct stat *sstat;
    sstat = malloc(sizeof(struct stat));
    if (sstat == NULL) {
        printf("fatal error: stat() memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    stat(name, sstat);
    *size = sstat->st_size;
    free(sstat);

    FILE *file = fopen(name, "r");
    if (file == NULL) {
        printf("fatal error: open/read of %s fails\n",
               name);
        exit(EXIT_FAILURE);
    }

    unsigned rounded = (*size | 0x03) + 1;

    char *buf = (char *)calloc(rounded, sizeof(char));
    if (buf == NULL) {
        printf("fatal error: read buffer memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_read = read(fileno(file), buf, *size);
    if (*size != (unsigned)bytes_read) {
        printf("fatal error: read failed, wanted to read %d bytes but only read %d\n",
               *size, (unsigned)bytes_read);
        exit(EXIT_FAILURE);
    }

    fclose(file);
    return (void *)buf;
}
