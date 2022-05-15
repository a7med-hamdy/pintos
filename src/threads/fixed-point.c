#include "fixed-point.h"
#include <inttypes.h>

struct real
int_to_real(int n){
    struct real res;
    res.val = n*F;
    return res;
}

int 
real_to_int(struct real x)
{
    if(x.val >= 0)
        return (x.val + F/2) / F;
    else
        return (x.val - F/2) / F;
}

struct real 
add_real_real(struct real x, struct real y)
{
    struct real res;
    res.val = x.val + y.val;
    return res;
}

struct real 
add_real_int(struct real x, int n)
{
    struct real res;
    res.val = x.val + n*F;
    return res;
}

struct real 
subtract_real_real(struct real x, struct real y)
{
    struct real res;
    res.val = x.val - y.val;
    return res;
}

struct real 
subtract_real_int(struct real x, int n)
{
    struct real res;
    res.val = x.val - n*F;
    return res;
}


struct real 
multiply_real_real(struct real x, struct real y)
{
    struct real res;
    res.val = ((int64_t)x.val)* y.val / F;
    return res;

}

struct real 
multiply_real_int(struct real x, int n)
{
    struct real res;
    res.val = x.val*n;
return res;
}

struct real 
divide_real_real(struct real x, struct real y)
{
    struct real res;
    res.val = ((int64_t) x.val) * F / y.val;
    return res;

}

struct real 
divide_real_int(struct real x, int n)
{
    struct real res;
    res.val = x.val / n;
    return res;

}