// ── Test file: exercises all Mini-C++ features the VM must support ──

// 1. Variables & arithmetic
int main() {
    int x = 10;
    int y = 20;
    int sum = x + y;
    int diff = x - y;
    int prod = x * y;
    int quot = y / x;

    // 2. Boolean & comparisons
    bool flag = true;
    bool check = x == y;
    bool less = x < y;

    // 3. If / else
    if (x < y) {
        sum = sum + 1;
    } else {
        sum = sum - 1;
    }

    // 4. While loop
    int counter = 0;
    while (counter < 5) {
        counter = counter + 1;
    }

    // 5. For loop
    int total = 0;
    for (int i = 0; i < 10; i++) {
        total = total + i;
    }

    // 6. Nested if
    if (flag == true) {
        if (x < 100) {
            x = x + 1;
        }
    }

    return 0;
}

// 7. Functions with parameters
int add(int a, int b) {
    return a + b;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
