all:
	g++ main.cpp -lgvc -lcgraph -o mop ;
	mv ./mop ~/.local/bin

