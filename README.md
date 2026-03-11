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