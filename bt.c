/**
 * balanced ternary
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef unsigned long ulong;

struct ternary {
	ulong n;
	ulong p;
};

typedef struct ternary T;

T encode(ulong n)
{
	T t = { 0, 0 };
	size_t ti = 0; /* trit index */

	while (n > 0) {
		switch (n % 3) {
			case 1:
				t.p |= 1UL << ti;
				n--;
				break;
			case 2:
				t.n |= 1UL << ti;
				n++;
				break;
			default:
				;
		}
		/* 3 | n */
		n /= 3;
		/* base-3 left shift */
		ti++;
	}

	return t;
}

size_t trit_size(T t)
{
	ulong np = t.n | t.p;

	if (np == 0) {
		return 0;
	}

	return 64 - (size_t)__builtin_clzl(np);
}

size_t trit_zero(T t)
{
	ulong np = t.n | t.p;

	return (size_t)__builtin_popcountl(~np);
}

int parity(T t)
{
	return trit_zero(t) & 1;
}

void print(T t)
{
	size_t ti = 63;
	while (ti != (size_t)-1) {
		if (t.n & (1UL << ti)) {
			putchar('-');
		} else if (t.p & (1UL << ti)) {
			putchar('+');
		} else {
			putchar('0');
		}
		ti--;
	}

	printf(" (tritsize=%lu zeros=%lu parity=%i)", trit_size(t), trit_zero(t), parity(t));

	putchar('\n');
}

ulong decode(T t)
{
	ulong n = 0;
	size_t ti = 63;
	while (ti != (size_t)-1) {
		n *= 3;

		/* assert: t is normalized */
		if (t.n & (1UL << ti)) {
			n--;
			t.n -= 1UL << ti; /* reset trit */
		}
		if (t.p & (1UL << ti)) {
			n++;
			t.p -= 1UL << ti; /* reset trit */
		}

		ti--;
	}

	return n;
}

T mul3(T t)
{
	t.n <<= 1;
	t.p <<= 1;

	return t;
}

T div3(T t)
{
	t.n >>= 1;
	t.p >>= 1;

	return t;
}

T div_pow3(T t, size_t k)
{
	t.n >>= k;
	t.p >>= k;

	return t;
}

int is_normalized(T t)
{
	return (t.n & t.p) == 0;
}

int is_nonzero(T t)
{
	return t.n != 0 || t.p != 0;
}

/* https://en.wikipedia.org/wiki/Balanced_ternary#Multi-trit_addition_and_subtraction */
T add(T a, T b)
{
	T c = b; /* carry */

	while (is_nonzero(c)) {
		/* a += c */

		T a_, c_;

		a_.n = (a.p & c.p) | (a.n & ~(c.n | c.p)) | (c.n & ~(a.n | a.p));
		a_.p = (a.n & c.n) | (a.p & ~(c.n | c.p)) | (c.p & ~(a.n | a.p));

		c_.n = (a.n & c.n) << 1;
		c_.p = (a.p & c.p) << 1;

		a = a_;
		c = c_;
	}

	return a;
}

T neg(T a)
{
	T a_;

	a_.n = a.p;
	a_.p = a.n;

	return a_;
}

T sub(T a, T b)
{
	T b_;

	b_.n = b.p;
	b_.p = b.n;

	return add(a, b_);
}

T mul2(T t)
{
	return add(t, t);
}

T mul4(T t)
{
	return mul2(mul2(t));
}

T mul16(T t)
{
	return mul4(mul4(t));
}

T mul32(T t)
{
	return mul2(mul16(t));
}

/* http://homepage.divms.uiowa.edu/~jones/ternary/multiply.shtml#div2 */
T div2(T t)
{
	T acc = t;

	acc = add(acc, div_pow3(acc, 32));
	acc = add(acc, div_pow3(acc, 16));
	acc = add(acc, div_pow3(acc, 8));
	acc = add(acc, div_pow3(acc, 4));
	acc = add(acc, div_pow3(acc, 2));
	acc = add(acc, div_pow3(acc, 1));

	acc = div_pow3(acc, 1);

	/* correction term */
	{
		T d = sub(t, mul2(acc));

		if (d.n < d.p) {
			acc = add(acc, encode(1));
		}

		if (d.n > d.p) {
			acc = sub(acc, encode(1));
		}
	}

	return acc;
}

T div8(T t)
{
	T acc = t;

	acc = add(acc, div_pow3(acc, 32));
	acc = add(acc, div_pow3(acc, 16));
	acc = add(acc, div_pow3(acc, 8));
	acc = add(acc, div_pow3(acc, 4));
	acc = add(acc, div_pow3(acc, 2));

	return div_pow3(acc, 2);
}

T div32_stub(T t)
{
	T acc = t;

	acc = add(acc, div_pow3(acc, 32));
	acc = add(acc, div_pow3(acc, 16));
	acc = add(acc, div_pow3(acc, 8));
	acc = add(acc, div_pow3(acc, 4));

	acc = div_pow3(acc, 3);

	return acc;
}

T div32(T t)
{
	T acc = t;
	T d; /* difference t - t/32*32 */

	acc = add(acc, div_pow3(acc, 32));
	acc = add(acc, div_pow3(acc, 16));
	acc = add(acc, div_pow3(acc, 8));
	acc = add(acc, div_pow3(acc, 4));

	acc = div_pow3(acc, 3);

	/* correction term */
	while (is_nonzero(d = sub(t, mul32(acc)))) {
		acc = add(acc, div32_stub(d));
	}

	return acc;
}

T tabs(T t)
{
	if (t.p >= t.n) {
		return t;
	} else {
		return neg(t);
	}
}

int less_than(T t, ulong b)
{
	T tb = encode(b);
	T d = sub(t, tb);

	return d.p < d.n;
}

T any_div32(T t)
{
	T acc = t;
	T d; /* difference t - t/32*32 */

	acc = add(acc, div_pow3(acc, 32));
	acc = add(acc, div_pow3(acc, 16));
	acc = add(acc, div_pow3(acc, 8));
	acc = add(acc, div_pow3(acc, 4));

	acc = div_pow3(acc, 3);

	/* correction term */
	while (1) {
		d = sub(t, mul32(acc));

		if ( less_than(tabs(d), 32) ) {
			break;
		}

		acc = add(acc, div32_stub(d));
	}

	return acc;
}

void test()
{
	ulong n;

	for (n = 0; n < 10000; ++n) {
		assert(is_normalized(encode(n)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(n == decode(encode(n)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(3*n == decode(mul3(encode(n))));
	}

	for (n = 0; n < 10000; ++n) {
		assert(n % 2 == (ulong)parity(encode(n)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(n+1+2*n == decode(add(encode(n+1), encode(2*n))));
	}

	for (n = 0; n < 10000; ++n) {
		assert((2*n+3)-(n+1) == decode(sub(encode(2*n+3), encode(n+1))));
	}

	for (n = 0; n < 10000; ++n) {
		assert(2*n == decode(mul2(encode(n))));
	}

	for (n = 0; n < 100000; ++n) {
		assert((long)(n/32) - (long)(decode(any_div32(encode(n)))) <= 1);
	}

	for (n = 0; n < 1000000; n += 2) {
		assert(n/2 == decode(div2(encode(n))));
	}

	for (n = 0; n < 10000000; n += 8) {
		assert(n/8 == decode(div8(encode(n))));
	}

	for (n = 0; n < 10000000; n += 32) {
		assert(n/32 == decode(div32(encode(n))));
	}
}

int main()
{
	ulong n;

	test();

	for (n = 0; n < 50; ++n) {
		printf("n = %2lu: ", n);
		print(encode(n));
	}

	return 0;
}
