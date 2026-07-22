CXX := g++-16
CXXFLAGS := -std=c++26 -freflection

a.out: main.cpp NewType.dt
	$(CXX) $(CXXFLAGS) main.cpp -o a.out

run: a.out
	./a.out

clean:
	rm -f a.out

.PHONY: run clean