stalk: s-talk.o list.o
	gcc -g s-talk.o list.o -pthread -o s-talk
	
stalk.o: s-talk.c
	gcc -c s-talk.c
	
list.o: list.h
	
clean:
	rm *.o s-talk

