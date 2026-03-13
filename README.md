# av

`av` is a low-level language which I'm simultaneously trying to both define and create a working compiler for.

You can find dev logs and language specification in `/spec`.

# Installation

To install `av` on macOS or Linux, run:

```bash
curl -fsSL https://avighnac.com/av/install | bash
```

For windows users, nagivate over to `https://github.com/avighnac/av/releases` and download the `.exe` file for the release of your choice.

---

The compiler's developed sufficiently to be able to compile the following `.av` code:
```av
int32 write(int32 fileno, void* mem, int32 bytes);
int32 main() {
  int8 c = 65_8;
  int8* ptr = &c;
  int32 succ = write(1, ptr, 1);
  return 0;
}
```

It generates the following assembly:
```asm
section .text
main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov eax, 65
  mov byte[rbp-1], al
  lea rax, [rbp-1]
  mov qword[rbp-9], rax
  mov eax, 1
  mov edi, eax
  mov rax, qword[rbp-9]
  mov rsi, rax
  mov eax, 1
  mov edx, eax
  call write
  mov dword[rbp-13], eax
  mov eax, 0
  jmp main.end
main.end:
  mov rsp, rbp
  pop rbp
  ret
write:    
  mov eax, 1
  syscall
  ret
global _start
_start:
  call main
  mov edi, eax
  mov eax, 60
  syscall
```

Which is pretty great!

# Features

The language supports the following compiler intrinsics:

- `int32 write(int32 fileno, void* mem, int32 bytes)`: writes bytes from `mem` to `fileno`
- `int32 read(int32 fileno, void* mem, int32 bytes)`: reads bytes from `mem` to `fileno`
- `void* alloca(int32 bytes)`: allocates `bytes` (rounded up to a multiple of 16) bytes on the stack

The following language features are supported:

- `if` statements, written like so:

```av
if (condition) {

}
```

- `while` loops:

```
whlie (condition) {

}
```

where `condition` is true if it is not equal to `0`.

- Bitwise AND and OR (written like `a | b` and `a & b`).
- Comparasions (<, >, <=, >=, etc.)
- Arithmetic (+, -, *, %, etc.)

Note that operator precedence may not work how you expect it to (definitely a feature). For example, this:

`n * (n + 1) / 2`

Evaluates to `n * ((n + 1) / 2)`, and not the expected `(n * (n + 1)) / 2`, for which you'd need parentheses.

The following types exist in the language:

- `int8`, `int16`, `int32`, `int64`: self explanatory, signed integers with fixed width.
- `int8*`, `int16*`, `int32*`, `int64*`: pointers to the above types. Keep in mind that `int8 *x` is **invalid**, you will need to do `int8* x` instead.
- `void*` also exists and can be freely cast to any of the other pointers. For example:

```av
void* mem = alloca(10);
int8* ptr = mem;
```

Note that this means `int` isn't a type and you'll need to use `int32` instead.

For signed literals, the following syntax is also supported:

```av
int8 c = 48_8;
```

If the prefix is excluded, the default is a 32-bit literal.

The language has a weak type system: this means you can cast pretty much anything to anything without the compiler complaining. But be warned: with great power comes great responsibility.

Each program needs the definition of `int32 main()` or it will fail to compile. To bypass this (for example, if you're just interested in the assembly for a particular function), use the `--s` flag while compiling.

# Known issues

- Global variables currently don't work.

# License

This project is released under the MIT License.

