# Zephyr Programming Language

Zephyr is a statically, strongly typed language which compiles to assembly.

## *Zephyr is currently WIP and highly likely to change - use at your own risk!*

## Installation / Bootstrapping

As Zephyr is self-hosted, it must be [bootstrapped](https://en.wikipedia.org/wiki/Bootstrapping_(compilers)) first.

The assembly files in the ./bootstrap directory can be used for this.

Firstly, you must have [yasm](https://yasm.tortall.net/) installed into your `$PATH`.

```console
$ yasm -felf64 -o ./bootstrap/bootstrap_x86_64_linux ./bootstrap/bootstrap_x86_64_linux.yasm
$ ld -o ./bootstrap/bootstrap_x86_64_linux ./bootstrap/bootstrap_x86_64_linux.o

$ ./bootstrap/bootstrap_x86_64_linux -o ./build/zephyr ./src/main.zpr

$ ./build/zephyr -o ./build/zephyr ./zpr/main.zpr
$ ./build/zephyr -o ./build/zephyr ./zpr/main.zpr
$ ./build/zephyr -o ./build/zephyr ./zpr/main.zpr
$ ...
```

## Examples

### Hello, World
```
import "std/io.zpr";

function main(): int {
	putsln("Hello, World!");
	return 0;
}
```

### File Write
```
import "std/io.zpr";

function main(): int {
	var file = fopen("./myfile.txt", 'w');
	file.putsln("Hello, Zephyr FileIO!");
	file.close();
	return 0;
}
```

### More Examples

See ./examples for more examples of the language.

## Testing

Running the bash scripts in ./tests can be used to test the language.

Running ./tests/all.sh will run all the tests and give a count of the
successful, total, and skipped tests.