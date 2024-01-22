In order to run the code, run make command and then:
1. sudo insmod partb_1_20CS30045_20CS30024.ko
2. lsmod | grep partb_1_20CS30045_20CS30024 (to check if module is loaded)
3. cd tests
4. gcc test1.c -o test1 && ./test1
5. dmesg (to check the output of printk statements)
6. sudo rmmod partb_1_20CS30045_20CS30024 (to remove the module)

Our testcases test positive as well as negative numbers and see if dequeueing is done correctly or not. All memory deallocation is also done correctly.
In test1.c, we check +ve, -ve inputs
In test2.c, we check multi-process dequeueing
In test3.c, we check if the same process can open the procfile multiple times