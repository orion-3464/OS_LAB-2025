savedcmd_/home/user/shared/lunix/lunix.o := ld -m elf_x86_64 -z noexecstack   -r -o /home/user/shared/lunix/lunix.o @/home/user/shared/lunix/lunix.mod  ; ./tools/objtool/objtool --hacks=jump_label --hacks=noinstr --hacks=skylake --ibt --orc --retpoline --rethunk --static-call --uaccess --prefix=16  --link  --module /home/user/shared/lunix/lunix.o

/home/user/shared/lunix/lunix.o: $(wildcard ./tools/objtool/objtool)
