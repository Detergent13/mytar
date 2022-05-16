#include "header.h"

#define HEADER_PADDING 12
#define CHKSUM_START 148
#define CHKSUM_END 155

/* We pass the header as a char to make pointer arithmetic easy. */
int calc_checksum(unsigned char *h) {
    int total = 0;
    int i;

    for(i = 0; i < (sizeof(struct header) - HEADER_PADDING); i++) {
        /* If we're within the indices of the chksum, treat it as a space. */
        if(i < CHKSUM_START || i > CHKSUM_END) {
            /* Make sure that it's interpreted as unsigned */
            total += (unsigned char)(*h);
            h++;
        }
        /* Otherwise, add the current octet to running total */
        else {
            total += ' ';
            h++;
        }
    }
    
    return total;
}
