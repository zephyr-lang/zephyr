#!/usr/bin/env bash

. tests/asserts.sh

echo -n "Returns: "
assert_exit_code 0 "function main(): int { return 0; }"
assert_exit_code 100 "function main(): int { return 100; }"
assert_compilation_error "function main(): int {}"
assert_stdout "5" "function x() { printu(5); } function main(): int { x(); return 0; }"
assert_exit_code 10 "function main(): int { if(1) { return 10; } else { return 5; } }"
assert_exit_code 10 "function main(): int { if(1) return 10; else return 5; }"
assert_exit_code 0 "function x() { return; } function main(): int { x(); return 0; }"
assert_compilation_error "function main(): int { return; }"
echo " Done"

echo -n "Literals: "
assert_exit_code 65 "function main(): int { return 'A'; }"
assert_compilation_error "function main(): int { return 'AZ'; }"
assert_exit_code 72 "function main(): int { return \"Hello\"[0]; }"
assert_stdout "72
101
108
108
111" "
function main(): int {
	var s = \"Hello\";

	for(var i = 0; s[i]; i = i + 1) {
		printu(s[i]);
	}

	return 0;
}
"
echo " Done"

echo -n "Unary: "
assert_exit_code 255 "function main(): int { return -1; }" # Unsigned
assert_exit_code 0 "function main(): int { return !56; }"
assert_exit_code 1 "function main(): int { return !0; }"
assert_exit_code 253 "function main(): int { return ~2; }" # Unsigned
assert_exit_code 0 "function main(): int { return !~0; }"
echo " Done"

echo -n "Arithmetic: "
assert_exit_code 8 "function main():int { return 5 + 3; }"
assert_exit_code 3 "function main():int { return 12 - 9; }"
assert_exit_code 56 "function main():int { return 8 * 7; }"
assert_exit_code 12 "function main():int { return 37 / 3; }"
assert_exit_code 1 "function main():int { return 37 % 3; }"
assert_exit_code 20 "function main():int { return 5 + 3 * 5; }"
assert_exit_code 40 "function main():int { return (5 + 3) * 5; }"
echo " Done"

echo -n "Bitwise: "
assert_exit_code 2 "function main():int { return 15 & 2; }"
assert_exit_code 27 "function main():int { return 26 | 3; }"
assert_exit_code 21 "function main():int { return 16 ^ 5; }"
assert_exit_code 40 "function main():int { return 5 << 3; }"
assert_exit_code 3 "function main():int { return 214 >> 6; }"
echo " Done"

echo -n "Relative: "
assert_exit_code 1 "function main():int { return 15 > 2; }"
assert_exit_code 0 "function main():int { return 16 < 6; }"
assert_exit_code 1 "function main():int { return 15 >= 15; }"
assert_exit_code 0 "function main():int { return 16 <= 2; } "
assert_exit_code 1 "function main():int { return 25 == 25; }"
assert_exit_code 0 "function main():int { return 6 != 6; }"
echo " Done"

echo -n "Local Variables: "
assert_exit_code 5 "function main():int { var a:int = 5; return a; }"
assert_exit_code 16 "function main():int { var a:int = 6; var b:int = 10; return a + b; }"
assert_exit_code 7 "function main():int { var a: int; var b:int = 7; return b; }"
assert_exit_code 12 "function main():int { var a: int; a = 12; return a; }"
assert_exit_code 24 "function main():int { var a:int = 6; var b:int = 10; a = b = 12; return a + b; }"
assert_stdout "5
5
5" "function main():int { var a = 5; var b = 0; var c = b = a; printu(a); printu(b); printu(c); return 0; }"
echo " Done"

echo -n "Functions: "
assert_exit_code 12 "function x():int { return 12; } function main():int { return x(); }"
assert_exit_code 11 "function x(y: int, z: int): int { return y + z; } function main(): int { return x(5, 6); }"
assert_compilation_error "function x():int {} function main():int { return x(1); }"
assert_compilation_error "function x(){} function main():int { return x(); }"

assert_stdout "1
2
3
4
5
6
7" "
function z(a1: int, a2: int, a3: int, a4: int, a5: int, a6: int, a7: int) {
	printu(a1);
	printu(a2);
	printu(a3);
	printu(a4);
	printu(a5);
	printu(a6);
	printu(a7);
}

function main(): int {
	z(1,2,3,4,5,6,7);
	return 0;
}
"

assert_stdout "99" "
function add(a: int, b: int): int {
	return a + b;
}

function mul(a: int, b: int): int {
	return a * b;
}

function main(): int {
	printu(mul(add(5, 6), add(3, 6)));
	return 0;
}
"

echo " Done"

echo -n "Typing: "
assert_compilation_error "function x() {} function main():int { return 12 ^ x(); }"
assert_compilation_error "function main(): int { var x; return 0; }"
assert_exit_code 10 "function main(): int { var x: int; x = 10; return x; }"
assert_exit_code 0 "
function main(): int {
	var i = 10;
	var ip = &i;

	var it: int = i;
	var ipt: int* = ip;

	return 0;
}
"
assert_compilation_error "function main(): int { 12 + 5; return 0; }"
echo " Done"

echo -n "Scoping: "
assert_exit_code 150 "
function main(): int {
	var x: int = 5;

	{
		var y: int = 6;

		{
			var x: int = 10;
			y = x;
		}

		var z: int = 15;
		var a: int = 16;

		return y * z;
	}
}
"

assert_compilation_error "
function main(): int {
	var x: int = 5;

	{
		var y: int;

		y = 56;

		var y: int;
	}

	return 0;
}
"

assert_exit_code 42 "
function x(a: int): int {
	var a: int = 42;
	return a;
}

function main(): int {
	return x(6);
}
"

echo " Done"

echo -n "Print: "
assert_stdout "42" "
function main(): int {
	printu(42);
	
	return 0;
}
"

assert_stdout "5
6" "
function main(): int {
	printu(5);
	printu(6);
	
	return 0;
}
"

assert_stdout "0" "
function main(): int {
	printu(0);
	return 0;
}
"

echo " Done"

echo -n "Pointers: "

assert_compilation_error "
function main(): int {
	var ip: int* = &10;
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = 10;
	var ip: int* = i;
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = 10;
	var ip: int** = &(&i);
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = 10;
	var ip: int* = &i;
	var ipp: int** = ip;
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = *10;
	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i: int = 10;
	var ip: int* = &i;
	var j: int = **ip;
	return 0;
}
"

assert_stdout "10" "
function main(): int {
	var i: int = 10;
	var ip: int* = &i;
	var j: int = *ip;
	printu(j);
	return 0;
}
"

assert_stdout "16
46868
388569403
49859385950595" "
function main(): int {
	var c: i8 = 16;
	var s: i16 = 46868;
	var i: i32 = 388569403;
	var l: i64 = 49859385950595;

	var cp: i8* = &c;
	var sp: i16* = &s;
	var ip: i32* = &i;
	var lp: i64* = &l;

	printu(*cp);
	printu(*sp);
	printu(*ip);
	printu(*lp);

	return 0;
}
"

assert_stdout "12" "
function main(): int {
	var i = 10;
	var ip = &i;

	*ip = 12;

	printu(*ip);

	return 0;
}
"

assert_compilation_error "
function main(): int {
	var i = 10;
	*i = 12;
	return 0;
}
"

assert_compilation_error "
function x() {}
function main(): int {
	var i = 10;
	var ip = &i;

	*ip = x();

	printu(*ip);

	return 0;
}
"

assert_exit_code 0 "
import \"std/core.zpr\";
function main(): int {
	var i = 0;
	var ip = &i;

	return ip == null;
}
"

echo " Done"

echo -n "Global Variables: "

assert_compilation_error "
var x: int = 10;
var x: i16;

function main(): int {
	return 0;
}
"

assert_exit_code 16 "
var x = 16;

function main(): int {
	return x;
}
"

assert_exit_code 56 "
var x: int;

function main(): int {
	x = 56;

	return x;
}
"

assert_stdout "16
46868
388569403
49859385950595" "
var c: i8 = 16;
var s: i16 = 46868;
var i: i32 = 388569403;
var l: i64 = 49859385950595;

function main(): int {
	var cp: i8* = &c;
	var sp: i16* = &s;
	var ip: i32* = &i;
	var lp: i64* = &l;

	printu(*cp);
	printu(*sp);
	printu(*ip);
	printu(*lp);

	return 0;
}
"

echo " Done"

echo -n "Sizeof: "

assert_stdout "1
2
4
8
8" "
function main(): int {
	printu(sizeof(i8));
	printu(sizeof(i16));
	printu(sizeof(i32));
	printu(sizeof(i64));
	printu(sizeof(int));

	return 0;
}
"

assert_stdout "16
8" "
struct Foo {
	y: int;
	z: int;
}

struct Bar {
	x: i8*;
}
function main(): int {
	printu(sizeof(Foo));
	printu(sizeof(Bar));
	return 0;
}
"

echo " Done"

echo -n "Syscall: "
assert_stdout "Hello, World!" "
function main(): int {	
	syscall3(1, 1, \"Hello, World!\", 13);

	var n: i8 = 10;
	syscall3(1, 1, &n, 1);

	return 0;
}
"

assert_compilation_error "
function main(): int {
	var x = syscall3(1, 1, \"Hello\", 5);

	printu(x);

	return 0;
}
" SKIP

echo " Done"

echo -n "Cast: "
assert_stdout "68" "
function main(): int {
	var i = 1094861636;
	var ip = &i;

	var c: i8 = i as i8;

	printu(c as int);

	return 0;
}
"
echo " Done"

echo -n "Struct: "
assert_exit_code 0 "
struct Item {
	next: Item*;
	value: int;
}

function main(): int { return 0; }
"

assert_compilation_error "
struct Point {
	p: Point;
}

function main(): int { return 0; }
"

assert_exit_code 0 "
struct Z {
	x: i32[2];
}

struct Y {
	y: i64;
}

function main(): int {
	var z: Z;
	var y: Y;
	z = y;
	return 0;
}
" SKIP

assert_compilation_error "
struct Foo {
	a: int;
}
struct Bar {
	b: int;
	c: int;
}

function main(): int {
	var f: Foo;
	var b: Bar;

	f = b;

	return 0;
}
"

assert_stdout "25" "
struct Point {
	x: int;
	y: int;
}

function main(): int {
	var p: Point;

	p.x = 12;
	p.y = 13;

	printu(p.x + p.y);

	return 0;
}
"

assert_stdout "25" "
struct Point {
	x: int;
	y: int;
	sum: int;
}

function sum(p: Point*) {
	p.sum = p.x + p.y;
}

function main(): int {
	var p: Point;

	p.x = 12;
	p.y = 13;

	sum(&p);

	printu(p.sum);

	return 0;
}
"

assert_compilation_error "
struct Point {
	x: int;
	y: int;
	sum: int;
}

function main(): int {
	var p: Point;
	var ip = &p;
	var ipp = &ip;

	ipp.x = 12;

	return 0;
}
"

assert_stdout "25" "
struct Point {
    x: int;
    y: int;
    sum: int;
}

function Point.calc_sum() {
    this.sum = this.x + this.y;
}

function main(): int {
    var p: Point;

    p.x = 12;
    p.y = 13;

    p.calc_sum();

    printu(p.sum); // 25

    return 0;
}
"

assert_stdout "2" "
struct Point {
	x: int;
	y: int;
}

struct Line {
	a: Point*;
	b: Point*;
}

function Line.gradient(): int {
	return (this.a.y - this.b.y) / (this.a.x - this.b.x);
}

function main(): int {
	var p1: Point;

	p1.x = 13;
	p1.y = 12;

	var p2: Point;

	p2.x = 10;
	p2.y = 6;

	var l: Line;
	l.a = &p1;
	l.b = &p2;

	printu(l.gradient());

	return 0;
}
"

assert_stdout "24" "
struct Point {
	x: int;
	y: int;
}

function double(y: int*) {
	*y = *y * 2;
}

function main(): int {
	var p: Point;

	p.x = 12;

	double(&p.x);

	printu(p.x);

	return 0;
}
"

assert_stdout "24" "
struct Point {
	x: int;
	y: int;
}

function double(y: int*) {
	*y = *y * 2;
}

function main(): int {
	var p: Point;

	p.x = 12;

	var pp = &p;

	double(&pp.x);

	printu(p.x);

	return 0;
}
"

assert_stdout "11
42" "
struct X {
	type: int;

	value: struct {
		c: i8;
		s: i16;
	};

}

function main(): int {
	var x: X;

	x.value.c = 42;

	printu(sizeof(X));
	printu(x.value.c);

	return 0;
}
"

echo " Done"

echo -n "Union: "

assert_stdout "2" "
union SInt {
	char: i8;
	short: i16;
}

function main(): int {
	printu(sizeof(SInt));

	return 0;
}
"

assert_stdout "12" "
union SInt {
	char: i8;
	short: i16;
}

function main(): int {
	var si: SInt;

	si.char = 12;

	printu(si.char as int);

	return 0;
}
"

assert_stdout "15" "
union SInt {
	char: i8;
	short: i16;
}

function main(): int {
	var si: SInt;
	var sip = &si;

	sip.char = 15;

	printu(sip.char as int);

	return 0;
}
"

assert_stdout "10
42" "
struct X {
	type: int;

	value: union {
		c: i8;
		s: i16;
	};

}

function main(): int {
	var x: X;

	x.value.c = 42;

	printu(sizeof(X));
	printu(x.value.c);

	return 0;
}
"

echo " Done"

echo -n "Copying: "

assert_stdout "10
12" "
struct Point {
	x: int;
	y: int;
}

function main(): int {
	var p1: Point;
	p1.x = 10;
	p1.y = 12;

	var p2: Point;
	p2 = p1;

	printu(p2.x);
	printu(p2.y);

	return 0;
}
"

assert_stdout "10
12
20
24" "
struct Point {
	x: int;
	y: int;
}

struct Line {
	a: Point;
	b: Point;
}

function main(): int {
	var p1: Point;
	p1.x = 10;
	p1.y = 12;

	var p2: Point;
	p2.x = 20;
	p2.y = 24;

	var l: Line;
	l.a = p1;
	l.b = p2;

	printu(l.a.x);
	printu(l.a.y);
	printu(l.b.x);
	printu(l.b.y);

	return 0;
}
"

assert_stdout "10
12
20
24" "
struct Point {
	x: int;
	y: int;
}

struct Line {
	a: Point;
	b: Point;
}

function main(): int {
	var p1: Point;
	p1.x = 10;
	p1.y = 12;

	var p2: Point;
	p2.x = 20;
	p2.y = 24;

	var l: Line;
	l.a = p1;
	l.b = p2;

	var l1: Line;
	l1 = l;

	printu(l1.a.x);
	printu(l1.a.y);
	printu(l1.b.x);
	printu(l1.b.y);

	return 0;
}
"

assert_stdout "10
12" "
struct Point {
	x: int;
	y: int;
}

function main(): int {
	var p: Point;
	p.x = 10;
	p.y = 12;

	var p1: Point;

	var pp = &p1;
	*pp = p;
	printu(pp.x);
	printu(pp.y);

	return 0;
}
"

assert_stdout "12
15" "
struct Point {
	x: int;
	y: int;
}

function main(): int {
	var p: Point;
	p.x = 12;
	p.y = 15;
	var pp = &p;

	var p1 = *pp;

	printu(p1.x);
	printu(p1.y);

	return 0;
}
"

assert_stdout "12
15" "
struct Point {
	x: int;
	y: int;
}

var p: Point;

function main(): int {
	p.x = 12;
	p.y = 15;
	var pp = &p;
	
	var p1 = *pp;

	printu(p1.x);
	printu(p1.y);

	return 0;
}
"

echo " Done"

echo -n "Constants: "

assert_stdout "12" "
const x = 12;

function main(): int {
	printu(x);
	return 0;
}
"

assert_compilation_error "
const x = 12;

function main(): int {
	x = 13;
	printu(x);
	return 0;
}
"

echo " Done"

echo -n "Enum: "

assert_stdout "0
1
2
3" "
enum Animal {
	DOG,
	CAT,
	ZEBRA,
	BEAR
}

function main(): int {
	printu(Animal::DOG);
	printu(Animal::CAT);
	printu(Animal::ZEBRA);
	printu(Animal::BEAR);
	return 0;
}
"

echo " Done"

echo -n "Argv: "

assert_stdout_argv "hello
world" "
import \"std/io.zpr\";
function main(argc: int, argv: i8**): int {
	for(var i = 1; i < argc; i = i + 1) {
		putsln(argv[i]);
	}
	return 0;
}
" hello world

echo " Done"

echo -n "Assignment: "

assert_stdout "X
50" "
import \"std/io.zpr\";

struct X {
	y: int;
}

var x: X;

function get_x(): X* {
	putsln(\"X\");
	return &x;
}

function main(): int {
	x.y = 5;

	get_x().y *= 10;

	printu(x.y);

	return 0;
}
"

assert_stdout "0
2
5
3" "
function main(): int {
	var x = 0;
	printu(x++);
	printu(++x);

	var y = 5;

	printu(y--);
	printu(--y);

	return 0;
}
"

echo " Done"

echo -n "Namespace: "

assert_stdout "Hello, World!" "
import \"std/io.zpr\";
namespace hello {
	namespace world {

		var message = \"Hello, World!\";

		function print() {
			putsln(hello::world::message);
		}
	}
}

function main(): int {
	hello::world::print();

	return 0;
}
"

assert_stdout "22" "
import \"std/io.zpr\";

namespace math {
	struct Point {
		x: int;
		y: int;
	}

	function math::Point.sum(): int {
		return this.x + this.y;
	}
}

function main(): int {
	var point: math::Point;
	point.x = 10;
	point.y = 12;

	putd(point.sum()); putln();

	return 0;
}
"

assert_stdout "30" "
import \"std/io.zpr\";

namespace math {
	struct Point {
		x: int;
		y: int;
	}

	function Point.sum(): int {
		return this.x + this.y;
	}
}

function main(): int {
	var point: math::Point;
	point.x = 14;
	point.y = 16;

	putd(point.sum()); putln();

	return 0;
}
"

assert_stdout "Hello, World!" "
import \"std/io.zpr\";

namespace hello {
	var message = \"Hello, World!\";
	namespace world {
		function print() {
			putsln(message);
		}
	}
}

function main(): int {  hello::world::print(); return 0; }
"

assert_stdout "10" "
import \"std/io.zpr\";

namespace hello {
	const x = 10;
	namespace world {
		function print() {
			putd(x);
			putln();
		}
	}
}

function main(): int {
	hello::world::print();

	return 0;
}
"

assert_stdout "42" "
import \"std/io.zpr\";

namespace hello {
	function x(): int {
		return 42;
	}

	namespace world {
		function print() {
			putd(x());
			putln();
		}
	}
}

function main(): int {
	hello::world::print();

	return 0;
}
"

assert_stdout "0
0
1" "
import \"std/io.zpr\";

enum Type {
	A,
	B
}

function main(): int {
	var type: Type = Type::A;

	putd(type);
	putln();
	putd(Type::A);
	putln();
	putd(Type::B);
	putln();

	return 0;
}
"

assert_stdout "Y!" "
import \"std/io.zpr\";

namespace x {
	const y = 10;
}

function main(): int {
	var a = 10;

	when(a) {
		x::y -> putsln(\"Y!\");
		else -> putsln(\"Not y\");
	}

	return 0;
}
"

assert_stdout "A" "
import \"std/io.zpr\";

enum Type {
	A,
	B
}

function main(): int {
	var type: Type = Type::A;

	when(type) {
		Type::A -> putsln(\"A\");
		Type::B -> putsln(\"B\");
	}

	return 0;
}
"

echo " Done"

echo -n "Floats: "

assert_stdout "10" "
function main(): int {
	var x = 10 as f64;
	var y = x as int;

	printu(y);

	return 0;
}
"

assert_stdout "20" "
function main(): int {
	var x = 10 as f64;
	x += x;

	printu(x as int);

	return 0;
}
"

assert_stdout "25" "
function sum(a: f64, b: f64): f64 {
	return a + b;
}

function main(): int {
	var x = sum(10 as f64, 15 as f64);

	printu(x as int);

	return 0;
}
"

assert_stdout "10" "
var x: f64;

function main(): int {
	x = 10 as f64;

	printu(x as int);

	return 0;
}
"

assert_stdout "2.5" "
import \"std/io.zpr\";

function main(): int {
	var x = 5 as f64;
	x /= 2 as f64;

	putft(x);

	return 0;
}
"

assert_stdout "10.0, 15.0" "
import \"std/io.zpr\";
struct Point {
	x: f64;
	y: f64;
}

function Point.println() {
	putft(this.x);
	puts(\", \");
	putft(this.y);
	putln();
}

function main(): int {
	var point: Point;
	point.x = 10 as f64;
	point.y = 15 as f64;

	point.println();

	return 0;
}
"

assert_stdout "(20.0, 25.0)" "
import \"std/io.zpr\";

struct Point {
	x: f64;
	y: f64;
}

function Point.translate(dx: f64, dy: f64) {
	this.x += dx;
	this.y += dy;
}

function Point.println() {
	puts(\"(\");
	putft(this.x);
	puts(\", \");
	putft(this.y);
	putsln(\")\");
}

function main(): int {
	var point: Point;
	point.x = 15 as f64;
	point.y = 10 as f64;

	point.translate(5 as f64, 15 as f64);

	point.println();

	return 0;
}
"

assert_stdout "0.0
1.0
2.0
3.0
4.0" "
import \"std/io.zpr\";

function main(): int {
	var arr: f64[5];

	for(var i = 0; i < 5; ++i) {
		arr[i] = i as f64;
	}

	for(var i = 0; i < 5; ++i) {
		putft(arr[i]);
		putln();
	}

	return 0;
}
"

assert_stdout "0.0
1.0
2.0
3.0
4.0" "
import \"std/io.zpr\";

function main(): int {
	var arr: f64[5] = [ 0 as f64, 1 as f64, 2 as f64, 3 as f64, 4 as f64 ];

	for(var i = 0; i < 5; ++i) {
		putft(arr[i]);
		putln();
	}

	return 0;
}
"

assert_stdout "0.0
1.0
2.0
3.0
4.0" "
import \"std/io.zpr\";

var arr: f64[5] = [ 0 as f64, 1 as f64, 2 as f64, 3 as f64, 4 as f64 ];

function main(): int {

	for(var i = 0; i < 5; ++i) {
		putft(arr[i]);
		putln();
	}

	return 0;
}
"

assert_stdout "1
0
1
0
0
1" "
function main(): int {

	var fiveHalves = 5 as f64;
	fiveHalves /= 2 as f64;

	var two = 2 as f64;

	printu(fiveHalves > two);
	printu(fiveHalves < two);
	printu(fiveHalves >= two);
	printu(fiveHalves <= two);
	printu(fiveHalves == two);
	printu(fiveHalves != two);

	return 0;
}
"

assert_stdout "2.5" "
import \"std/io.zpr\";

function main(): int {
	var x = 2.5;

	putft(x);
	putln();

	return 0;
}
"

assert_stdout "12.5" "
import \"std/io.zpr\";

function main(): int {
	var x = 2.5;

	x += 10;

	putft(x); putln();

	return 0;
}
"

assert_stdout "10.0" "
import \"std/io.zpr\";

function main(): int {
	var x = 2.5;

	x = 10;

	putft(x); putln();

	return 0;
}
"

assert_stdout "2" "
import \"std/io.zpr\";

function main(): int {
	var x = 2.5;
	var y: i64;
	y = x;

	putd(y); putln();

	return 0;
}
"

assert_stdout "10.0" "
import \"std/io.zpr\";


function main(): int {
	var x: f64 = 10;

	putft(x); putln();

	return 0;
}
"

assert_stdout "10.0" "
import \"std/io.zpr\";

var x: f64 = 10;

function main(): int {

	putft(x); putln();

	return 0;
}
"

assert_stdout "10" "
import \"std/io.zpr\";

function main(): int {
	var x: int = 10.5;

	putd(x); putln();

	return 0;
}
"

assert_stdout "10" "
import \"std/io.zpr\";

var x: int = 10.5;

function main(): int {

	putd(x); putln();

	return 0;
}
"

echo " Done"

echo -n "Unsigned: "

assert_stdout "1" "
function main(): int {
	var x: uint = -1;
	printu(x > 1);
	return 0;
}
"

assert_stdout "9223372036854775807" "
function main(): int {
	var x: uint = -1;
	printu(x / 2);
	return 0;
}
"

assert_stdout "20" "
function main(): int {
	var x = 20u;
	printu(x);
	return 0;
}
"

echo " Done"