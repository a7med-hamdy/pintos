#define F 16384 // 2^14

/* fixed point representation of integer*/
struct real{
    int val;
};

struct real int_to_real(int n);
int real_to_int(struct real x);
struct real add_real_real(struct real x, struct real y);
struct real add_real_int(struct real x, int n);
struct real subtract_real_real(struct real x, struct real y);
struct real subtract_real_int(struct real x, int n);
struct real multiply_real_real(struct real x, struct real y);
struct real multiply_real_int(struct real x, int n);
struct real divide_real_real(struct real x, struct real y);
struct real divide_real_int(struct real x, int n);