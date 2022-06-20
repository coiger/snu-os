#include <stdio.h>
#include <stdint.h>

#define M 1000000
#define PI 3141592
#define RAD 57295779
#define RADIUS 6400000000
#define _1 (M)
#define _2 (2*M)
#define _3 (3*M)
#define _4 (4*M)
#define _5 (5*M)
#define _6 (6*M)
#define ADD(x, y) (x + y)
#define SUB(x, y) (x - y)
#define MUL(x, y) (((int64_t) x) * y / M)
#define DIV(x, y) (((int64_t) x) * M / y)
#define SQR(x) (MUL(x, x))

// Cosine macro is defined only for 0~90.
#define COS(x) (fixed_cos_table[MUL(x, RAD) / M])

typedef int fixed32_t;

static fixed32_t fixed_cos_table[] = {
	1000000, 999848, 999391, 998630, 997564, // 0
        996195, 994522, 992546, 990268, 987688,  // 5
        984808, 981627, 978148, 974370, 970296,  // 10
        965926, 961262, 956305, 951057, 945519,  // 15
        939693, 933580, 927184, 920505, 913545,  // 20
        906308, 898794, 891007, 882948, 874620,  // 25
        866025, 857167, 848048, 838671, 829038,  // 30
        819152, 809017, 798636, 788011, 777146,  // 35
        766044, 754710, 743145, 731354, 719340,  // 40
        707107, 694658, 681998, 669131, 656059,  // 45
        642788, 629320, 615661, 601815, 587785,  // 50
        573576, 559193, 544639, 529919, 515038,  // 55
        500000, 484810, 469472, 453990, 438371,  // 60
        422618, 406737, 390731, 374607, 358368,  // 65
        342020, 325568, 309017, 292372, 275637,  // 70
        258819, 241922, 224951, 207912, 190809,  // 75
        173648, 156434, 139173, 121869, 104528,  // 80
         87156,  69756,  52336,  34899,  17452,  // 85
             0                                   // 90
};

void FROM_FIXED_INTEGER(int* integer, fixed32_t x) {
	if (x < 0) {
		*integer = x / M - 1;
	} else {
		*integer = x / M;
	}
}

void FROM_FIXED_FRACTION(unsigned int* fraction, fixed32_t x) {
	*fraction = (x % M + M) % M;
}

void FROM_FIXED(int* integer, unsigned int* fraction, fixed32_t x) {
	FROM_FIXED_INTEGER(integer, x);
	FROM_FIXED_FRACTION(fraction, x);
}

fixed32_t TO_FIXED(int integer, unsigned int fraction) {
	return integer * M + fraction;
}

int main() {
	int integer, frac;	
	fixed32_t a, b, c, d;

	a = TO_FIXED(9, 200000);
	FROM_FIXED(&integer, &frac, a);
	printf("%d+0.%d\n", integer, frac);

	b = TO_FIXED(-9, 200000);
	FROM_FIXED(&integer, &frac, b);
	printf("%d+0.%d\n", integer, frac);

	c = MUL(a, b);
	FROM_FIXED(&integer, &frac, c);
	printf("%d+0.%d\n", integer, frac);

	d = COS(DIV(PI, _4));
	FROM_FIXED(&integer, &frac, d);
	printf("%d+0.%d\n", integer, frac);

	return 0;
}
