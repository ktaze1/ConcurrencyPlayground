#include <iostream>
#include <thread>

void hello(){
	std::cout << "Hello from thread\n";
}

int main() {
	std::cout << "Hello from Clang!\n";

	std::thread t(hello); // declare t of type std::thread
	// t is executed concurrently
	t.join(); // when finished, join main thread.
}