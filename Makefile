serv1 serv2 serv3 client talk: serv1.c serv2.c serv3.c client.c talk.c

	gcc -o serv1  serv1.c  
	gcc -o serv2  serv2.c
	gcc -o serv3  serv3.c
	gcc -o client client.c
	gcc -o talk   talk.c

