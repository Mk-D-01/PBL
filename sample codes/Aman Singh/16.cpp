#include <iostream>
using namespace std;

// Unary operators overloaded using friend functions
class Number {
    int x, y;
public:
    Number(int a = 0, int b = 0) : x(a), y(b) {}

    friend Number operator-(const Number &n);        // unary minus
    friend Number &operator++(Number &n);            // pre-increment
    friend Number operator++(Number &n, int);        // post-increment
    friend Number &operator--(Number &n);            // pre-decrement
    friend Number operator--(Number &n, int);        // post-decrement

    void display(const char *label) const { cout << label << " = (" << x << ", " << y << ")\n"; }
};

Number operator-(const Number &n) { return Number(-n.x, -n.y); }

// Friends take the object by non-const reference since they modify it
Number &operator++(Number &n) { ++n.x; ++n.y; return n; }
Number operator++(Number &n, int) { Number old = n; ++n.x; ++n.y; return old; }
Number &operator--(Number &n) { --n.x; --n.y; return n; }
Number operator--(Number &n, int) { Number old = n; --n.x; --n.y; return old; }

int main() {
    Number n(5, -3);
    n.display("n      ");
    Number m = -n;
    m.display("-n     ");

    Number a = ++n;  a.display("++n (a)"); n.display("n      ");
    Number b = n++;  b.display("n++ (b)"); n.display("n      ");
    Number c = --n;  c.display("--n (c)"); n.display("n      ");
    Number d = n--;  d.display("n-- (d)"); n.display("n      ");
    return 0;
}
