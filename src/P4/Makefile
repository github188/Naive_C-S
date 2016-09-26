all : devmod netmod

devmod:
	arm-hisiv100nptl-linux-gcc -W -o Dev_mod dev_mod.c p4ledfun.c\
		-I ./include/ -L ./lib/ -lnmea -lpthread -liconv

netmod:
	arm-hisiv100nptl-linux-gcc -W -o Net_mod net_mod.c \
		-I ./include/ -L ./lib/ -lpthread

clean :
	rm Dev_mod Net_mod
