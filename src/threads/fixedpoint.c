#include "fixedpoint.h"
#include <inttypes.h>

real
int_to_real(int n){
    real res;
    res.val = n*f;
    return res;
}

int 
real_to_int(real x)
{
    if(x.val >= 0)
        return (x.val + f/2) / f;
    else
        return (x.val - f/2) / f;
}

real 
add_real_real(real x, real y)
{
    real res;
    res.val = x.val + y.val;
    return res;
}

real 
add_real_int(real x, int n)
{
    real res;
    res.val = x.val + n*f;
    return res;
}

real 
subtract_real_real(real x, real y)
{
    real res;
    res.val = x.val - y.val;
    return res;
}

real 
subtract_real_int(real x, int n)
{
    real res;
    res.val = x.val - n*f;
    return res;
}


real 
multiply_real_real(real x, real y)
{
    real res;
    res.val = ((int64_t)x.val)* y.val / f;
    return res;

}

real 
multiply_real_int(real x, int n)
{
    real res;
    res.val = x.val*n;
return res;
}

real 
divide_real_real(real x, real y)
{
    real res;
    res.val = ((int64_t) x.val) * f / y.val;
    return res;

}

real 
divide_real_int(real x, int n)
{
    real res;
    res.val = x.val / n;
    return res;

}


