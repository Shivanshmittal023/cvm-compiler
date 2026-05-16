// Full integration test — exercises the entire pipeline

int add(int a, int b) {
    return a + b;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    // 1. Arithmetic
    int x = 10;
    int y = 20;
    print(x + y);       // expect: 30
    print(x - y);       // expect: -10
    print(x * y);       // expect: 200
    print(y / x);       // expect: 2

    // 2. Function calls
    print(add(3, 7));    // expect: 10
    print(factorial(6)); // expect: 720

    // 3. Boolean & comparison
    bool flag = true;
    if (flag == true) {
        print(1);        // expect: 1
    }
    if (x < y) {
        print(2);        // expect: 2
    }

    // 4. While loop
    int counter = 0;
    while (counter < 5) {
        counter = counter + 1;
    }
    print(counter);      // expect: 5

    // 5. For loop
    int sum = 0;
    for (int i = 1; i <= 10; i++) {
        sum = sum + i;
    }
    print(sum);          // expect: 55

    // 6. Nested if
    int val = 0;
    if (x < 100) {
        if (y > 10) {
            val = 99;
        }
    }
    print(val);          // expect: 99

    // 7. Unary negate
    int neg = -42;
    print(neg);          // expect: -42

    return 0;
}
