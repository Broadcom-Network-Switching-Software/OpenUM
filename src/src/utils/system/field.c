#include <system.h>


#define DEBUG 1

#if DEBUG 
#define LOCAL_DEBUG_PRINTF(args ...)  do { sal_printf("%s %d:", __FUNCTION__, __LINE__); sal_printf(args); } while(0)
#else
#define LOCAL_DEBUG_PRINTF(args ...)  
#endif


/*
 * Function:
 *    field_get
 * Purpose:
 *    Extract multi-word field value from multi-word register/memory.
 * Parameters:
 *    entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 *      fbuf - buffer where to store extracted field value
 * Returns:
 *      Pointer to extracted field value.
 */
uint32 *field_get(const uint32 *entbuf, int sbit, int ebit, uint32 *fbuf)
{
    int i, wp, bp, len;
    uint32 mask;

    bp = sbit;
    len = ebit - sbit + 1;


    wp = bp / 32;
    bp = bp & (32 - 1);
    i = 0;

    for (; len > 0; len -= 32, i++) {
        if (i > ((ebit - sbit + 1)/32)) {
           LOCAL_DEBUG_PRINTF("trash\n");
        } 
        if (bp) {
            if (len < 32) {
                mask = (1 << len) - 1;
            } else {
                mask = ~0;
            }
            fbuf[i] = (entbuf[wp++] >> bp) & mask;
            if (len > (32 - bp)) {
                fbuf[i] |= ((entbuf[wp] & (mask >> (32 - bp))) << (32 - bp));
            } 
        } else {
            fbuf[i] = entbuf[wp++];
            if (len < 32) {
                fbuf[i] &= ((1 << len) - 1);
            }
        }
    }

    return fbuf;
}

/*
 * Function:
 *    field_set
 * Purpose:
 *    Assign multi-word field value in multi-word register/memory.
 * Parameters:
 *    entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 *      fbuf - buffer with new field value
 * Returns:
 *      Nothing.
 */
void field_set(uint32 *entbuf, int sbit, int ebit, uint32 *fbuf)
{
    uint32 mask;
    int i, wp, bp, len, cp;

    bp = sbit;
    len = ebit - sbit + 1;

    cp = (ebit) / 32;
    wp = bp / 32;
    bp = bp & (32 - 1);
    i = 0;
    for (; len > 0; len -= 32, i++) {
        if (cp < wp) {
            LOCAL_DEBUG_PRINTF("trash\n");
        }
        if (bp) {
            if (len < 32) {
                mask = (1 << len) - 1;
            } else {
                mask = ~0;
            }
            entbuf[wp] &= ~(mask << bp);
            entbuf[wp] |= ((fbuf[i] & mask) << bp);
            wp++;
            if (len > (32 - bp)) {
                entbuf[wp] &= ~(mask >> (32 - bp));
                entbuf[wp] |= ((fbuf[i] >> (32 - bp)) & (mask >> (32 -bp)));
            } 
        } else {
            if (len < 32) {
                mask = (1 << len) - 1;
                entbuf[wp] &= ~mask;
                entbuf[wp] |= (fbuf[i] & mask);
            } else {
                entbuf[wp] = fbuf[i];
            }
            wp++;
        }
    }

}

uint32 field32_get(const uint32 *entbuf, int sbit, int ebit) {
                                
        uint32 ret;
        field_get(entbuf, sbit, ebit, &ret);

        return ret;


}
void field32_set(uint32 *entbuf, int sbit, int ebit, uint32 fval) {      
     field_set(entbuf, sbit, ebit, &fval);
}




