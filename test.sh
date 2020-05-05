#!/bin/bash
assert() {
	expected="$1"
	input="$2"

	./9cc "$input" > tmp.s
	cc -o tmp tmp.s
	./tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$input => $expected expected, but got $actual"
		exit 1
	fi
}
assert 24 "return 24;"
assert 18 "num = 3; return num * num * 2;"
assert 32 "w = 6; h = 2; p = 2; return (w + p) * (h + p);"
assert 32 "w=6;h=2;w=w+2;h=h+2;return w*h;"

assert 42 "foo=20; bar=22; foo+bar;"
assert 6 "one = 1; two = 2; three = 3; one * two * three;"
assert 15 "a=1; a+14;"
assert 1 "a = 1; a  + 1 - + a;"
assert 34 "a=3; b = 7; a * 2 + 4 * b;"
assert 12 "a = 3; b = 2; 4 * a * 2 / b;"
assert 1 "a = 31; a / a;"

assert 0 "0==1;"
assert 1 "42 == 42;"
assert 1 "0!=1;"
assert 0 "42 != 42;"
assert 1 "0<1;"
assert 0 "1 < 1;"
assert 0 "2<1;"
assert 1 "0 <= 1;"

assert 47 "5+6*7;"
assert 15 "5*(9-6);"
assert 4 "(3+5)/2;"
assert 10 "-10+20;"
assert 10 "- -10;"
assert 10 "- - + 10;"

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5; "

echo OK
