#include <stdio.h>
int add(int a, int b) {
    return a + b;
}
int main() {
    int x = 10;
    int y = 20;
    int z = add(x, y);
    if (z > 20) {
        printf("Result is greater than 20\n");
    } else {
        printf("Result is 20 or less\n");
    }
    return 0;
}