all: lazybox cp cd ls stat ln cat chown chownPerm lazyshell

lazybox: lazybox.o
	gcc -g -static lazybox.o -o lazybox

cp:
	ln -s lazybox cp

ls:
	ln -s lazybox ls

ln:
	ln -s lazybox ln

cat:
	ln -s lazybox cat

cd:
	ln -s lazybox cd

stat:
	ln -s lazybox stat

chown:
	ln -s lazybox chown

chownPerm:
	sudo chmod +x chownPerm.sh
	sudo ./chownPerm.sh

lazyshell:
	ln -s lazybox lazyshell

clean:
	rm -f *.o lazybox cp cd stat ls ln cat chown lazyshell sudo_success