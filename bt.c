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

/* signed decode */
long sdecode(T t)
{
	return is_positive(t) ? (long)decode(t) : -(long)decode(neg(t));
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

int leq_than(T a, T b)
{
	T d = sub(a, b);

	return d.p <= d.n;
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

size_t spow2(size_t k)
{
	return (size_t)1 << k;
}

/* http://homepage.divms.uiowa.edu/~jones/ternary/multiply.shtml#div2 */
T div_2_k_stub(T t, size_t k)
{
	size_t m = (k + 1) / 2;
	size_t r = spow2((k - 1) / 2);
	size_t s;

	T acc = t;

	assert(k % 2 == 1);

	for (s = r; s < sizeof(ulong) * 8; s *= 2) {
		acc = add(acc, div_pow3(acc, s));
	}

	acc = div_pow3(acc, m);

	return acc;
}

T div32_stub(T t)
{
	return div_2_k_stub(t, 5);
}

T div_2_k_slow(T t, size_t k)
{
	T d; /* difference t - t / 2^k * 2^k */
	T acc = div_2_k_stub(t, k);
	T c; /* correction term */

	d = sub(t, mul_2_k(acc, k));
	c = tabs(d); /* overkill */
	c = div_2_k_stub(c, k);

	/* correction */
	while (is_nonzero(d = sub(t, mul_2_k(acc, k)))) {
		if (is_positive(d)) {
		    acc = add(acc, add(c, encode(1)));
		} else {
		    acc = sub(acc, add(c, encode(1)));
		}
		c = div3(c);
	}

	return acc;
}

T div_2_k(T t, size_t k)
{
	T d; /* difference t - t / 2^k * 2^k */
	T acc = div_2_k_stub(t, k);

	/* correction term */
	if (is_nonzero(d = sub(t, mul_2_k(acc, k)))) {
		acc = add(acc, div_2_k_slow(d, k));
	}

	return acc;
}

T approx_div_2_k(T t, size_t k)
{
	T acc = div_2_k_stub(t, k);
#if 0
	T d; /* difference t - t / 2^k * 2^k */

	/* correction term */
	if (is_nonzero(d = sub(t, mul_2_k(acc, k)))) {
		acc = add(acc, div_2_k_stub(d, k));
	}
#endif
	return acc;
}

T approx_mod_2_k(T t, size_t k)
{
	T acc = div_2_k_stub(t, k);
#if 0
	T d; /* difference t - t / 2^k * 2^k */

	/* correction term */
	if (is_nonzero(d = sub(t, mul_2_k(acc, k)))) {
		acc = add(acc, div_2_k_stub(d, k));
	}
#endif
	return sub(t, mul_2_k(acc, k));
}

/* EXPERIMENTAL */
T mod_2_k_1(T t, size_t k)
{
	T acc = t;
	T d;
	T m = encode((1UL << k) - 1); /* modulus */

	do {
		T d_ = encode(10000000); /* FIXME */
		if (less_than(tabs(acc), m)) {
			if (!is_positive(acc)) {
				acc = add(acc, m);
			}
			return acc;
		}
		acc = div_2_k_stub(acc, k); /* acc = acc / 2^k */
#if 1
		/* correction term */
		while (!leq_than(tabs(d = sub(t, mul_2_k(acc, k))), m)) {
			if (leq_than(tabs(d_), tabs(d))) {
				/* avoid divergence */
				break;
			}
			acc = add(acc, div_2_k_stub(d, k));
			d_ = d;
		}
#endif
		d = sub(t, mul_2_k(acc, k)); /* d = acc % 2^k */

		acc = add(acc, d);
#if 1
		/* over modulus */
		while (!less_than(acc, m)) {
			acc = sub(acc, m);
		}
#endif
	} while (1);
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
	size_t k;

	printf("test: n\n");
	for (n = 0; n < 10000; ++n) {
		assert(n == decode(encode(n)));
	}

	printf("test: is_normalized(n)\n");
	for (n = 0; n < 10000; ++n) {
		assert(is_normalized(encode(n)));
	}

	printf("test: parity(n)\n");
	for (n = 0; n < 10000; ++n) {
		assert(n % 2 == (ulong)parity(encode(n)));
	}

	printf("test: 3 * n\n");
	for (n = 0; n < 10000; ++n) {
		assert(3 * n == decode(mul3(encode(n))));
	}

	printf("test: n / 3\n");
	for (n = 0; n < 10000; n += 3) {
		assert(n / 3 == decode(div3(encode(n))));
	}

	printf("test: 9 * n\n");
	for (n = 0; n < 10000; ++n) {
		assert(9 * n == decode(mul_pow3(encode(n), 2)));
	}

	printf("test: n / 9\n");
	for (n = 0; n < 10000; n += 9) {
		assert(n / 9 == decode(div_pow3(encode(n), 2)));
	}

	printf("test: n + m\n");
	for (n = 0; n < 1000; ++n) {
		for (m = 0; m < 1000; ++m) {
			assert(n + m == decode(add(encode(n), encode(m))));
		}
	}

	printf("test: n - m\n");
	for (n = 0; n < 1000; ++n) {
		for (m = n; m < 1000; ++m) {
			assert(n - m == decode(sub(encode(n), encode(m))));
		}
	}

	printf("test: 2 * n\n");
	for (n = 0; n < 10000; ++n) {
		assert(2 * n == decode(mul_2_k(encode(n), 1)));
	}

	printf("test: 4 * n\n");
	for (n = 0; n < 10000; ++n) {
		assert(4 * n == decode(mul_2_k(encode(n), 2)));
	}

	printf("test: 8 * n\n");
	for (n = 0; n < 10000; ++n) {
		assert(8 * n == decode(mul_2_k(encode(n), 3)));
	}

	printf("test: 16 * n\n");
	for (n = 0; n < 10000; ++n) {
		assert(16 * n == decode(mul_2_k(encode(n), 4)));
	}

	printf("test: 32 * n\n");
	for (n = 0; n < 10000; ++n) {
		assert(32 * n == decode(mul_2_k(encode(n), 5)));
	}
#if 0
	printf("test: n / 2\n");
	for (n = 0; n < 1000000; n += 2) {
		assert(n / 2 == decode(div_2_k(encode(n), 1)));
	}

	printf("test: n / 8\n");
	for (n = 0; n < 10000000; n += 8) {
		assert(n / 8 == decode(div_2_k(encode(n), 3)));
	}

	printf("test: n / 32\n");
	for (n = 0; n < 10000000; n += 32) {
		assert(n / 32 == decode(div_2_k(encode(n), 5)));
	}

	printf("test: n / 128\n");
	for (n = 0; n < 10000000; n += 128) {
		assert(n / 128 == decode(div_2_k(encode(n), 7)));
	}

	printf("test: n / 512\n");
	for (n = 0; n < 10000000; n += 512) {
		assert(n / 512 == decode(div_2_k(encode(n), 9)));
	}

	printf("test: floor(n / 32)\n");
	for (n = 0; n < 100000; ++n) {
		assert(n/32 == decode(floor_div32(encode(n))));
	}

	printf("test: mod(n / 32)\n");
	for (n = 0; n < 100000; ++n) {
		assert(n%32 == decode(floor_mod32(encode(n))));
	}
#endif
	for (k = 1; k < 10; k += 2) {
		printf("test: n / %lu * %lu + n %% %lu\n", spow2(k), spow2(k), spow2(k));
		for (n = 0; n < 100000; ++n) {
			T q = approx_div_2_k(encode(n), k);
			T r = approx_mod_2_k(encode(n), k);
			assert(n == decode(add(mul_2_k(q, k), r)));
		}
	}

	/* very slow for k = 1 */
	for (k = 3; k < 10; k += 2) {
		ulong M = (1UL << k) - 1;
		printf("test: n %% %lu\n", M);
		for (n = 0; n < 100000; ++n) {
			assert( n % M == decode(mod_2_k_1(encode(n), k)) );
		}
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
