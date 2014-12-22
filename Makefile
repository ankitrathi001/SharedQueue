obj-m:= Squeue.o

all:
	make -C /lib/modules/$(shell uname -r)/build -I $(PWD) M=$(PWD) modules

clean:
	rm -f *.ko 
	rm -f *.o 
	rm -f Module.symvers 
	rm -f modules.order 
	rm -f *.mod.c 
	rm -rf .tmp_versions 
	rm -f *.mod.c 
	rm -f *.mod.o 
	rm -f \.*.cmd 
	rm -f Module.markers
