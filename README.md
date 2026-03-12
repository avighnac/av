# av

`av` is a low-level language which I'm simultaneously trying to both define and create a working compiler for.

You can find dev logs and language specification in `/spec`.

---

The compiler's developed sufficiently to be able to compile the following `.av` code:
```av
int32 write(int32 fileno, void* mem, int32 bytes);
int32 main();
int32 main() {
  int8 c;
  c = 65_8;
  int8* ptr;
  ptr = &c;
  int32 succ;
  succ = write(1, ptr, 1);
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
  jmp main__end
main__end:
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

