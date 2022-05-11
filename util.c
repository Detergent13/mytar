#define HEADER_PADDING 12
#define CHKSUM_START 148
#define CHKSUM_END 155

/* Simple sum of all octets in the header. Treat the checksum as <SPACE> chars
 * when calculating it. */
int calc_checksum(struct header *h) {
    int total = 0;
    int i;

    for(int i = 1; i <= (sizeof(struct header) - HEADER_PADDING); i++) {
        /* If we're within the indices of the chksum, treat it as a space. */
        if(i >= CHKSUM_START && i <= CHKSUM_END) {
            total += ' ';
            buffer++;
        }
        /* Otherwise, add the current octet to running total */
        else {
            total += *buffer;
            buffer++;
        }
    }
    
    return total;
}
