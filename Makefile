all: clean build run
build:
	cd src && g++ gg.cpp -o gg 
	mkdir -p build && mv src/gg build/
run: build
		./build/gg
clean:
	rm -rf build/
