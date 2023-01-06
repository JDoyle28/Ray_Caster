raycast: raycast.c v3math.c raycast.h v3math.h
	gcc raycast.c v3math.c -o raycast

clean:
	rm -rf *.o *.exe *.exe.stackdump
