savedcmd_/home/user/shared/lunix/lunix.mod := printf '%s\n'   lunix-module.o lunix-chrdev.o lunix-ldisc.o lunix-protocol.o lunix-sensors.o | awk '!x[$$0]++ { print("/home/user/shared/lunix/"$$0) }' > /home/user/shared/lunix/lunix.mod
