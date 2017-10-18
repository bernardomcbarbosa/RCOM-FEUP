gcc -Wall app_layer.c -c
gcc -Wall data_layer.c -c
gcc -Wall main.c -o main data_layer.o app_layer.o
scp -r /root/Desktop/tp3 netedu@192.168.109.25:/home/netedu/Desktop
