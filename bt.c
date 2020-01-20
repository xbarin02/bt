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

/* left shift by 1 */
T mul3(T t)
{
	t.n <<= 1;
	t.p <<= 1;

	return t;
}

/* left shift */
T mul_pow3(T t, size_t k)
{
	t.n <<= k;
	t.p <<= k;

	return t;
}

/* right shift by 1 */
T div3(T t)
{
	t.n >>= 1;
	t.p >>= 1;

	return t;
}

/* right shift */
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

T neg(T a)
{
	T a_;

	a_.n = a.p;
	a_.p = a.n;

	return a_;
}

/* absolute value */
T tabs(T t)
{
	if (t.p >= t.n) {
		return t;
	} else {
		return neg(t);
	}
}

/* is positive or zero */
int is_positive(T t)
{
	return t.p >= t.n;
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

T sub(T a, T b)
{
	T b_;

	b_.n = b.p;
	b_.p = b.n;

	return add(a, b_);
}

int less_than(T a, T b)
{
	T d = sub(a, b);

	return d.p < d.n;
}

T mul2(T t)
{
	return add(t, t);
}

/* t * 2^k */
T mul_2_k(T t, size_t k)
{
	if (k % 2 == 1) {
		return mul2(mul_2_k(t, k - 1));
	}

	if (k == 0) {
		return t;
	}

	return mul_2_k(mul_2_k(t, k / 2), k / 2);
}

T mul32(T t)
{
	return mul_2_k(t, 5);
}

T div2_stub(T t)
{
	T acc = t;

	acc = add(acc, div_pow3(acc, 32));
	acc = add(acc, div_pow3(acc, 16));
	acc = add(acc, div_pow3(acc, 8));
	acc = add(acc, div_pow3(acc, 4));
	acc = add(acc, div_pow3(acc, 2));
	acc = add(acc, div_pow3(acc, 1));

	acc = div_pow3(acc, 1);

	return acc;
}

T div8_stub(T t)
{
	T acc = t;

	acc = add(acc, div_pow3(acc, 32));
	acc = add(acc, div_pow3(acc, 16));
	acc = add(acc, div_pow3(acc, 8));
	acc = add(acc, div_pow3(acc, 4));
	acc = add(acc, div_pow3(acc, 2));

	acc = div_pow3(acc, 2);

	return acc;
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

/* http://homepage.divms.uiowa.edu/~jones/ternary/multiply.shtml#div2 */
T div2(T t)
{
	T acc = div2_stub(t);

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
	T acc = div8_stub(t);

	return acc;
}

T div32(T t)
{
	T d; /* difference t - t/32*32 */
	T acc = div32_stub(t);

	/* correction term */
	while (is_nonzero(d = sub(t, mul32(acc)))) {
		acc = add(acc, div32_stub(d));
	}

	return acc;
}

T floor_div32(T t)
{
	T d; /* difference t - t/32*32 */
	T acc = div32_stub(t);

	/* correction term */
	while (1) {
		d = sub(t, mul32(acc));

		if (less_than(tabs(d), encode(32))) {
			break;
		}

		acc = add(acc, div32_stub(d));
	}

	if (!is_positive(d)) {
		acc = sub(acc, encode(1));
		d = add(d, encode(32));
	}

	return acc;
}

T floor_mod32(T t)
{
	T d; /* difference t - t/32*32 */
	T acc = div32_stub(t);

	/* correction term */
	while (1) {
		d = sub(t, mul32(acc));

		if (less_than(tabs(d), encode(32))) {
			break;
		}

		acc = add(acc, div32_stub(d));
	}

	if (!is_positive(d)) {
		acc = sub(acc, encode(1));
		d = add(d, encode(32));
	}

	return d;
}

void test()
{
	ulong n, m;

	for (n = 0; n < 10000; ++n) {
		assert(is_normalized(encode(n)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(n == decode(encode(n)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(3 * n == decode(mul3(encode(n))));
	}

	for (n = 0; n < 10000; n += 3) {
		assert(n / 3 == decode(div3(encode(n))));
	}

	for (n = 0; n < 10000; ++n) {
		assert(n % 2 == (ulong)parity(encode(n)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(9 * n == decode(mul_pow3(encode(n), 2)));
	}

	for (n = 0; n < 10000; n += 9) {
		assert(n / 9 == decode(div_pow3(encode(n), 2)));
	}

	for (n = 0; n < 1000; ++n) {
		for (m = 0; m < 1000; ++m) {
			assert(n + m == decode(add(encode(n), encode(m))));
		}
	}

	for (n = 0; n < 1000; ++n) {
		for (m = n; m < 1000; ++m) {
			assert(n - m == decode(sub(encode(n), encode(m))));
		}
	}

	for (n = 0; n < 10000; ++n) {
		assert(2 * n == decode(mul2(encode(n))));
	}

	for (n = 0; n < 10000; ++n) {
		assert(2 * n == decode(mul_2_k(encode(n), 1)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(4 * n == decode(mul_2_k(encode(n), 2)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(8 * n == decode(mul_2_k(encode(n), 3)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(16 * n == decode(mul_2_k(encode(n), 4)));
	}

	for (n = 0; n < 10000; ++n) {
		assert(32 * n == decode(mul_2_k(encode(n), 5)));
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

	for (n = 0; n < 100000; ++n) {
		assert(n/32 == decode(floor_div32(encode(n))));
	}

	for (n = 0; n < 100000; ++n) {
		assert(n%32 == decode(floor_mod32(encode(n))));
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
