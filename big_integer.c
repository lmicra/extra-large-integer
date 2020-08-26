#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

typedef struct big_integer {
    size_t capacity;
    size_t digits;
    char *number;
} big_integer;

#define NUMBER_CHUNK_DIGITS 256

big_integer big_integer_zero = { 1, 1, "0" };
big_integer big_integer_one = { 1, 1, "1" };

size_t closest_chunk_multiple(size_t c) {
    return ((c+NUMBER_CHUNK_DIGITS)/NUMBER_CHUNK_DIGITS)*NUMBER_CHUNK_DIGITS + 2;
}

big_integer* big_integer_realloc(big_integer* this, size_t digit_inc) {
    size_t c = closest_chunk_multiple(this->digits + digit_inc);
    if(c != this->capacity) { // do we need to increase the capacity?
        // printf("%lu, %lu, %lu, %lu, %s\n", this->digits, this->digits + digit_inc, this->capacity, c, this->number);
        this->capacity = c;
        this->number = realloc(this->number, this->capacity);
        if(this->number == NULL) {
            free(this);
            this = NULL;
        } else
            fputs("realloc\n", stderr);
    }
    return this;
}

void inplace_reverse(char* s) {
    if(s) {
        char *start, *end;
        start = end = s;
        while(*end != 0)
            end++;
        end--;
        for(;start<end;start++,end--) {
            *start ^= *end;
            *end ^= *start;
            *start ^= *end;
        }
    }
}

void big_integer_print(big_integer* this) {
    inplace_reverse(this->number);
    fputs(this->number, stdout);
    printf(" (%lu/%lu)", this->digits, this->capacity);
    inplace_reverse(this->number);
}

void get_number_from_string(char *s, size_t *c, char *d) {
    char *s1=s, *s2=d;
    bool leading_zero = true;
    for(*c=0; *s1!=0; s1++) {
        if(*s1>='0' && *s1<='9') {
            if(leading_zero && *s1 == '0') continue;
            leading_zero = false;
            if(d!=NULL) *s2++ = *s1;
            *c += 1; // don't like syntax: ++*c
        }
    }
    if(d!=NULL) *s2=0;
}

big_integer* big_integer_from_string(char *s) {
    size_t c=0;
    get_number_from_string(s, &c, NULL);
    big_integer* this = malloc(sizeof(big_integer));
    if(this == NULL) return NULL;
    this->capacity = closest_chunk_multiple(c);
    this->number = malloc(this->capacity);
    if(this->number == NULL) {
        free(this);
        this = NULL;
        return this;
    }
    if(c==0) {
        this->digits = 1;
        this->number[0] = '0';
        this->number[1] = 0;
    } else {
        this->digits = c;
        get_number_from_string(s, &c, this->number);
        this->number[c] = 0;
    }
    inplace_reverse(this->number);
    return this;
}

void ltoa(size_t l, char *s, int base) {
    static const char alphanum[] = "0123456789ABCDEF";
    if(base<2 || base>16) return;
    size_t c=0,t;
    while(l>0 && c<21) {
        t = l % base;
        s[c++] = alphanum[t];
        l -= t;
        l /= 10;
    }
    s[c] = 0;
    inplace_reverse(s);
}

big_integer* big_integer_from_long(unsigned long l) {
    static char b[129];
    ltoa(l, b, 10);
    return big_integer_from_string(b);
}

void big_integer_free(big_integer* this) {
    free(this->number);
    free(this);
    this = NULL;
}

big_integer* big_integer_clone(big_integer* that) {
    big_integer* this = malloc(sizeof(big_integer));
    if(this == NULL) return NULL;
    this->digits = that->digits;
    this->capacity = closest_chunk_multiple(this->digits);
    this->number = malloc(this->capacity);
    if(this->number == NULL) {
        free(this);
        this = NULL;
        return this;
    }
    char *s1, *s2;
    for(s1=this->number,s2=that->number; *s2 != 0; s1++,s2++) *s1 = *s2;
    *s1 = 0;
    return this;
}

int big_integer_cmp(big_integer* this, big_integer* that) {
    if(this->digits != that->digits) return this->digits - that->digits;
    char *s1, *s2;
    for(s1=this->number, s2=that->number; *s1 == *s2 && *s1 != 0; s1++, s2++);
    return *s1 - *s2;
}

big_integer* big_integer_add(big_integer* this_input, big_integer* that_input) {
    // find the largest number and add the other
    big_integer *this, *that;
    if(big_integer_cmp(this_input, that_input) < 0) {
        this = big_integer_clone(that_input);
        that = this_input;
    } else {
        this = big_integer_clone(this_input);
        that = that_input;
    }

    size_t p=0;
    int n = 0;

    while(p < that->digits) {
        n += (this->number[p] - '0') + (that->number[p] - '0');
        this->number[p] = n % 10;
        this->number[p++] += '0';
        n = (n > 9 ? 1 : 0);
    }

    while(p < this->digits) {
        n += (this->number[p] - '0');
        this->number[p] = n % 10;
        this->number[p++] += '0';
        n = (n > 9 ? 1 : 0);
    }

    if(n == 1 && p >= this->digits) {
        this = big_integer_realloc(this, 1);
        this->digits++;
        this->number[p] = '1';
        this->number[p+1] = 0;
    }

    free(this_input->number);
    this_input->digits = this->digits;
    this_input->capacity = this->capacity;
    this_input->number = this->number;
    free(this);

    return this_input;
}

big_integer* big_integer_inc(big_integer* this) {
    return big_integer_add(this, &big_integer_one);
}

big_integer* big_integer_mul_10(big_integer* this, size_t times) {
    if(times == 0) return this;
    this = big_integer_realloc(this, times);
    inplace_reverse(this->number);
    for(size_t i=0; i<times; i++)
        this->number[this->digits++] = '0';
    this->number[this->digits] = 0;
    inplace_reverse(this->number);
    return this;
}

big_integer* big_integer_factor(big_integer* this, size_t times) {
    if(times == 0) {
        big_integer_free(this);
        this = big_integer_clone(&big_integer_zero);
        return this;
    }

    if (times == 1) {
        return this;
    }

    big_integer* that = big_integer_clone(this);
    for(size_t i=1; i<times; i++) {
        this = big_integer_add(this, that);
    }

    big_integer_free(that);

    return this;
}

big_integer* big_integer_mul(big_integer* this_input, big_integer* that_input) {
    big_integer *this, *that;
    if(big_integer_cmp(this_input, that_input) < 0) {
        this = that_input;
        that = this_input;
    } else {
        this = this_input;
        that = that_input;
    }

    big_integer* factors[10] = {};
    factors[0] = &big_integer_zero;
    for(size_t i=1; i<10; i++) {
        factors[i] = big_integer_clone(factors[i-1]);
        factors[i] = big_integer_add(factors[i], this);
    }

    big_integer* factor;
    big_integer* result = big_integer_clone(&big_integer_zero);
    for(size_t p=0; p<that->digits; p++) {
        if(that->number[p] != '0') {
            factor = big_integer_clone(factors[that->number[p]-'0']);
            factor = big_integer_mul_10(factor, p);
            result = big_integer_add(result, factor);
            big_integer_free(factor);
        }
    }

    for(size_t i=2; i<10; i++) {
        big_integer_free(factors[i]);
    }

    free(this_input->number);
    this_input->digits = result->digits;
    this_input->capacity = result->capacity;
    this_input->number = result->number;
    free(result);

    return this_input;
}

big_integer* big_integer_factorial(size_t n) {
    big_integer* result = big_integer_clone(&big_integer_one);
    if(n == 1) return result;
    big_integer* counter = big_integer_clone(&big_integer_one);
    for(size_t i=2; i<=n; i++) {
        counter = big_integer_inc(counter);
        result = big_integer_mul(result, counter);
    }
    big_integer_free(counter);
    return result;
}

void test01() {
    char v1[] = "1234567890";
    printf("%s\n", v1);
    inplace_reverse(v1);
    printf("%s\n", v1);
}

void test02() {
    size_t c;
    c=0; printf("%lu < %lu\n", c, closest_chunk_multiple(c));
    c=1; printf("%lu < %lu\n", c, closest_chunk_multiple(c));
    c=126; printf("%lu < %lu\n", c, closest_chunk_multiple(c));
    c=127; printf("%lu < %lu\n", c, closest_chunk_multiple(c));
    c=128; printf("%lu < %lu\n", c, closest_chunk_multiple(c));
}

void test03() {
    big_integer* v1 = big_integer_clone(&big_integer_zero);
    big_integer_print(v1); putchar('\n');
    big_integer_free(v1);
}

void test04() {
    big_integer* v1 = big_integer_from_string("0 _123");
    big_integer_print(v1); putchar('\n');
    big_integer_free(v1);
}

void test05() {
    char n[] = "1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
    char* s = n;
    big_integer* v1;
    for(int i=0; i<10; i++) {
        v1 = big_integer_from_string(s + i);
        big_integer_print(v1); putchar('\n');
        big_integer_free(v1);
    }
}

void test06() {
    big_integer* v1 = big_integer_clone(&big_integer_one);
    for(int i=0; i<150; i++) {
        v1 = big_integer_mul_10(v1, 2);
        big_integer_print(v1); putchar('\n');
    }
    big_integer_free(v1);
}

void test07() {
    big_integer* v1 = big_integer_clone(&big_integer_zero);
    for(int i=0; i<1000; i++) {
        v1 = big_integer_inc(v1);
        printf("%d: ", i+1); big_integer_print(v1); putchar('\n');
    }
    big_integer_free(v1);
}

void test08() {
    big_integer *v1, *v2;
    v1 = big_integer_from_string("9");
    for(int i=0; i<80; i++) {
        v2 = big_integer_clone(v1);
        v2 = big_integer_inc(v2);
        big_integer_print(v2); putchar('\n');
        big_integer_free(v2);
        v1 = big_integer_mul_10(v1, 1);
        v1->number[0] = '9';
    }
    big_integer_free(v1);
}

void test09() {
    big_integer* v0 = big_integer_from_string("18 446 744 073 709 551 615");
    big_integer* v1 = big_integer_from_string("003 2 936");
    big_integer* v2 = big_integer_from_string("32_936");
    big_integer* v3 = big_integer_from_string("32937");
    big_integer* v4 = big_integer_from_string("000_00  000 ___ 0");
    big_integer* v5 = big_integer_clone(&big_integer_one);

    fputs("v0: ", stdout); big_integer_print(v0); putchar('\n');
    fputs("v1: ", stdout); big_integer_print(v1); putchar('\n');
    fputs("v2: ", stdout); big_integer_print(v2); putchar('\n');
    fputs("v3: ", stdout); big_integer_print(v3); putchar('\n');
    fputs("v4: ", stdout); big_integer_print(v4); putchar('\n');
    fputs("v5: ", stdout); big_integer_print(v5); putchar('\n');

    printf("cmp: %d\n", big_integer_cmp(v5, v4));
    printf("cmp: %d\n", big_integer_cmp(v4, v5));
    printf("cmp: %d\n", big_integer_cmp(v1, v2));
}

void test10() {
    big_integer* zero = big_integer_clone(&big_integer_zero);
    big_integer_print(zero); putchar('\n');

    big_integer* one = big_integer_clone(zero);
    one = big_integer_inc(one);
    big_integer_print(one); putchar('\n');

    big_integer* v1 = big_integer_from_string("12345");
    v1 = big_integer_inc(v1);
    big_integer_print(v1); putchar('\n');

    big_integer_free(v1);
    big_integer_free(one);
    big_integer_free(zero);
}

void test11() {
    big_integer *v1, *v2;

    v1 = big_integer_from_string("0");
    big_integer_print(v1); putchar('\n');

    v2 = big_integer_clone(v1);
    big_integer_print(v2); putchar('\n');

    v2 = big_integer_inc(v2);
    big_integer_print(v2); putchar('\n');

    v1 = big_integer_add(v1, v2);
    big_integer_print(v1); putchar('\n');

    big_integer_free(v2);
    big_integer_free(v1);
}

void test12() {
    big_integer *v1, *v2;

//    v1 = big_integer_from_string("9876543210");
//    v2 = big_integer_from_string("0123456789");
//
//    v1 = big_integer_add(v1, v2);
//    big_integer_print(v1); putchar('\n');
//    big_integer_print(v2); putchar('\n');
//
//    big_integer_free(v2);
//    big_integer_free(v1);

    v1 = big_integer_from_string("984");
    v2 = big_integer_from_string("123");

    big_integer_print(v1); putchar('\n');
    big_integer_print(v2); putchar('\n');
    v1 = big_integer_add(v1, v2);
    big_integer_print(v1); putchar('\n');

    big_integer_free(v2);
    big_integer_free(v1);
}

void test13() {
    big_integer *v1, *v2;
    v1 = big_integer_clone(&big_integer_one);
    big_integer_print(v1); putchar('\n');
    for(size_t i=0; i<10; i++) {
        v2 = big_integer_clone(v1);
        v2 = big_integer_factor(v2, i);
        printf("%lu: ", i); big_integer_print(v2); putchar('\n');
        big_integer_free(v2);
    }
    big_integer_free(v1);
}

void test14() {
    big_integer *v1, *v2;
    v1 = big_integer_from_string("1234");
    big_integer_print(v1); putchar('\n');
    for(size_t i=0; i<10; i++) {
        v2 = big_integer_clone(v1);
        v2 = big_integer_factor(v2, i);
        printf("%lu: ", i); big_integer_print(v2); putchar('\n');
        big_integer_free(v2);
    }
    big_integer_free(v1);
}

void test15() {
    big_integer *v1, *v2;
    v1 = big_integer_from_string("9223372036854775807");
    v2 = big_integer_from_string("9223372036854775807");
    big_integer_print(v1); putchar('\n');
    big_integer_print(v2); putchar('\n');
    v1 = big_integer_mul(v1, v2);
    big_integer_print(v1); putchar('\n');
    big_integer_free(v2);
    big_integer_free(v1);
}

void test16() {
    big_integer *v1, *v2, *v3;
    v1 = big_integer_from_string(" 9223372036490439002");
    v2 = big_integer_from_string(" 9223372036709794588");
    v3 = big_integer_from_string("18446744073200233590");
//    v1 = big_integer_from_string("9223372036854775807");
//    v2 = big_integer_from_string("18446744073709551614");
//    v3 = big_integer_from_string("27670116110564327421");
    v1 = big_integer_add(v1, v2);
    if (big_integer_cmp(v1,v3) == 0) puts("OK");
    big_integer_print(v1); putchar('\n');
}

void test17() {
    big_integer *v1, *v2, *v3;
    v1 = big_integer_from_string("9223372036854775807922337203685477580792233720368547758079223372036854775807");
    v2 = big_integer_from_string("92233720368547758079223372036854775807922337203685477580792233720368547758079223372036854775807");
    v3 = big_integer_from_string("850705917302346158644110261302794244210314993087535387255621896004479135008936158678290238513375108808656000389751920484169874442691733008747414884640827396907784232501249");
    v1 = big_integer_mul(v1, v2);
    if (big_integer_cmp(v1,v3) == 0) puts("OK");
    big_integer_print(v1); putchar('\n');
}

void test18() {
    time_t t;
    size_t l1, l2, l3;
    srand((unsigned) time(&t));
    l1 = rand()+rand()+rand()+rand();
    l2 = rand()+rand()+rand()+rand();
    l1 /= 2;
    l2 /= 2;
    l3 = l1+l2;
    printf("  %20lu\n+ %20lu\n= %20lu\n", l1, l2, l3);

    big_integer *v1, *v2, *v3;
    v1 = big_integer_from_long(l1);
    v2 = big_integer_from_long(l2);
    v3 = big_integer_from_long(l3);
    v1 = big_integer_add(v1,v2);

    char *b = malloc(v1->digits+1);
    strcpy(b, v1->number);
    inplace_reverse(b);
    printf("= %20s\n", b);
    free(b);

    if (big_integer_cmp(v1,v3) == 0) puts("OK");

    big_integer_free(v1);
    big_integer_free(v2);
    big_integer_free(v3);
}

void test19() {
    big_integer *v1;

    v1 = big_integer_factorial(25);
    big_integer_print(v1); putchar('\n');
    big_integer_free(v1);

    v1 = big_integer_factorial(30);
    big_integer_print(v1); putchar('\n');
    big_integer_free(v1);

    v1 = big_integer_factorial(50);
    big_integer_print(v1); putchar('\n');
    big_integer_free(v1);

    v1 = big_integer_factorial(100);
    big_integer_print(v1); putchar('\n');
    big_integer_free(v1);
}

int main() {
    // test01();
    // test02();
    // test03();
    // test04();
    // test05();
    // test06();
    // test07();
    // test08();
    // test09();
    // test10();
    // test11();
    // test12();
    // test13();
    // test14();
    // test15();
    // test16();
    // test17();
    // test18();
    test19();
    return 0;
}
