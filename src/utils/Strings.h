#ifndef __STRINGS_H__
#define __STRINGS_H__

#define LE (1)

#define INVALID_CHAR_SUB ('_')

int isInvalidChar(char c)
{
    switch ( c )
    {
        case 0x3a: // : 
        case 0x21 : // !
        case 0x3f : // ?
        case 0x5c : // bs
        case 0x2f : // /
        case 0x22 : // "
        case 0x27 : // \'
        case 0x60 : // `
        case 0x24 : // $
        case 0x25 : // %
        case 0x26 : // &
        case 0x3c : // <
        case 0x3e : // >
        case 0x7c : // |
        case 0x5e : // ^
        case 0x7e : // ~
        case 0x23 : // #
            return 1;
        default:
            return 0;
    }
}

int sanitizePathA(char* in, size_t inSize, char* out, size_t outSize)
{
    if ( !in || !out )
        return -1;
    if ( inSize > outSize )
        return -2;

    for ( size_t i = 0; i < inSize; i++ )
    {
        char c = in[i];
        if ( isInvalidChar(c) )
            out[i] = INVALID_CHAR_SUB;
        else
            out[i] = c;
    }

    return 0;
}

/**
 * UTF8ToUTF16LE:
 * @outb:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @outb
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and try to convert it to an UTF-16LE
 * block of chars out.
 *
 * Returns the number of byte written, or -1 by lack of space, or -2
 *     if the transcoding failed.
 *
 * (c) https://dev.w3.org/XML/encoding.c
 */
int UTF8ToUTF16LE(unsigned char* outb, size_t* outlen, const unsigned char* in, size_t* inlen)
{
    unsigned short* out = (unsigned short*) outb;
    const unsigned char* processed = in;
    unsigned short* outstart = out;
    unsigned short* outend;
    const unsigned char* inend = in + *inlen;
    unsigned int c, d;
    int trailing;
    unsigned char* tmp;
    unsigned short tmp1, tmp2;

    if ( in == NULL )
    {
        //	initialization, add the Byte Order Mark
        if ( *outlen >= 2 )
        {
            outb[0] = 0xFF;
            outb[1] = 0xFE;
            *outlen = 2;
            *inlen = 0;
#ifdef DEBUG_ENCODING
            debug_info(xmlGenericErrorContext,
            "Added FFFE Byte Order Mark\n");
#endif
            return (2);
        }
        *outlen = 0;
        *inlen = 0;
        return (0);
    }
    outend = out + (*outlen / 2);
    while ( in < inend )
    {
        d = *in++;
        if ( d < 0x80 )
        {
            c = d;
            trailing = 0;
        }
        else if ( d < 0xC0 )
        {
            // trailing byte in leading position
            *outlen = (out - outstart) * 2;
            *inlen = processed - in;
            return (-2);
        }
        else if ( d < 0xE0 )
        {
            c = d & 0x1F;
            trailing = 1;
        }
        else if ( d < 0xF0 )
        {
            c = d & 0x0F;
            trailing = 2;
        }
        else if ( d < 0xF8 )
        {
            c = d & 0x07;
            trailing = 3;
        }
        else
        {
            // no chance for this in UTF-16
            *outlen = (out - outstart) * 2;
            *inlen = processed - in;
            return (-2);
        }

        if ( inend - in < trailing )
        {
            break;
        }

        for ( ; trailing; trailing-- )
        {
            if ((in >= inend) || (((d = *in++) & 0xC0) != 0x80))
                break;
            c <<= 6;
            c |= d & 0x3F;
        }

        // assertion: c is a single UTF-4 value
        if ( c < 0x10000 )
        {
            if ( out >= outend )
                break;
            if ( LE )
            {
                *out++ = (unsigned short)c;
            }
            else
            {
                tmp = (unsigned char*) out;
                *tmp = (unsigned char)c;
                *(tmp + 1) = (unsigned char)(c >> 8);
                out++;
            }
        }
        else if ( c < 0x110000 )
        {
            if ( out + 1 >= outend )
                break;
            c -= 0x10000;
            if ( LE )
            {
                *out++ = 0xD800 | (unsigned short)(c >> 10);
                *out++ = 0xDC00 | (c & 0x03FF);
            }
            else
            {
                tmp1 = 0xD800 | (unsigned short)(c >> 10);
                tmp = (unsigned char*) out;
                *tmp = (unsigned char) tmp1;
                *(tmp + 1) = tmp1 >> 8;
                out++;

                tmp2 = 0xDC00 | (c & 0x03FF);
                tmp = (unsigned char*) out;
                *tmp = (unsigned char) tmp2;
                *(tmp + 1) = tmp2 >> 8;
                out++;
            }
        }
        else
            break;
        processed = in;
    }
    *outlen = (out - outstart) * 2;
    *inlen = processed - in;
    return (0);
}

/**
 * UTF16LEToUTF8:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @inb:  a pointer to an array of UTF-16LE passwd as a byte array
 * @inlenb:  the length of @in in UTF-16LE chars
 *
 * Take a block of UTF-16LE ushorts in and try to convert it to an UTF-8
 * block of chars out. This function assume the endian properity
 * is the same between the native type of this machine and the
 * inputed one.
 *
 * Returns the number of byte written, or -1 by lack of space, or -2
 *     if the transcoding fails (for *in is not valid utf16 string)
 *     The value of *inlen after return is the number of octets consumed
 *     as the return value is positive, else unpredictiable.
 *
 * (c) https://dev.w3.org/XML/encoding.c
 */
int UTF16LEToUTF8(unsigned char* out, size_t* outlen, const unsigned char* inb, size_t* inlenb)
{
    unsigned char* outstart = out;
    const unsigned char* processed = inb;
    unsigned char* outend = out + *outlen;
    unsigned short* in = (unsigned short*) inb;
    unsigned short* inend;
    unsigned int c, d, inlen;
    unsigned char *tmp;
    int bits;

    if ((*inlenb % 2) == 1)
        (*inlenb)--;
    inlen = (unsigned int)(*inlenb / 2);
    inend = in + inlen;
    while ((in < inend) && (out - outstart + 5u < *outlen)) {
        if (LE) {
            c= *in++;
        } else {
            tmp = (unsigned char *) in;
            c = *tmp++;
            c = c | (((unsigned int)*tmp) << 8);
            in++;
        }
        if ((c & 0xFC00) == 0xD800) {    /* surrogates */
            if (in >= inend) {           /* (in > inend) shouldn't happens */
                break;
            }
            if (LE) {
                d = *in++;
            } else {
                tmp = (unsigned char *) in;
                d = *tmp++;
                d = d | (((unsigned int)*tmp) << 8);
                in++;
            }
            if ((d & 0xFC00) == 0xDC00) {
                c &= 0x03FF;
                c <<= 10;
                c |= d & 0x03FF;
                c += 0x10000;
            }
            else {
                *outlen = out - outstart;
                *inlenb = processed - inb;
                return(-2);
            }
        }

        /* assertion: c is a single UTF-4 value */
        if (out >= outend)
            break;
        if      (c <    0x80) {  *out++= (unsigned char)c;                bits= -6; }
        else if (c <   0x800) {  *out++= ((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
        else if (c < 0x10000) {  *out++= ((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
        else                  {  *out++= ((c >> 18) & 0x07) | 0xF0;  bits= 12; }

        for ( ; bits >= 0; bits-= 6) {
            if (out >= outend)
                break;
            *out++= ((c >> bits) & 0x3F) | 0x80;
        }
        processed = (const unsigned char*) in;
    }
    *outlen = out - outstart;
    *inlenb = processed - inb;
    return(0);
}

#endif
