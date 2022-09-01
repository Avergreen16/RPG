#include <iostream>
#include <cmath>

void collatz(int number, int& length_ref) {
    int length = 0;
    int mod_num = number;
    while(mod_num != 1) {
        if(mod_num % 2 == 0) {
            mod_num /= 2;
        } else {
            mod_num = 3 * mod_num + 1;
        }
        length++;
    }

    length_ref = length;
}

int main() {
    int largest_length = 0;
    int length_ref;
    for(int i = 2; i < 0x4000000; i++) {
        collatz(i, length_ref);
        if(length_ref > largest_length) {
            largest_length = length_ref;
            std::cout << i << ": " << length_ref << "\n";
        }
    }
}