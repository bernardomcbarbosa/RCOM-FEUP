#gcc -Wall app_layer.c -c
#gcc -Wall data_layer.c -c
#gcc -Wall main.c -o main data_layer.o app_layer.o
#scp -r /root/Desktop/tp3 netedu@192.168.109.25:/home/netedu/Desktop
scp data_layer.c data_layer.h app_layer.c app_layer.h main.c rcom@192.168.1.251:/home/rcom/Desktop/tp3
scp data_layer.c data_layer.h app_layer.c app_layer.h main.c rcom@192.168.1.252:/home/rcom/Desktop/tp3
