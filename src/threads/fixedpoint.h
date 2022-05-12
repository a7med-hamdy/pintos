# define f 16384 // 2^14

/* fixed point representation of integer*/
typedef struct real{
    int val;
}real;


real int_to_real(int n);
int real_to_int(real x);
real add_real_real(real x, real y);
real add_real_int(real x, int n);
real subtract_real_real(real x, real y);
real subtract_real_int(real x, int n);
real multiply_real_real(real x, real y);
real multiply_real_int(real x, int n);
real divide_real_real(real x, real y);
real divide_real_int(real x, int n);