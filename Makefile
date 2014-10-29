obj-m += mp2_rms.o
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc mp2_test.c -o mp2_test
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm mp2_test
