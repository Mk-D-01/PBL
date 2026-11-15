#include <iostream>
using namespace std;

// Unary operators overloaded as member functions
class Number {
    int x, y;
public:
    Number(int a = 0, int b = 0) : x(a), y(b) {}

    Number operator-() const { return Number(-x, -y); }       // unary minus

    Number &operator++() { ++x; ++y; return *this; }          // pre-increment
    Number operator++(int) { Number old = *this; ++x; ++y; return old; }  // post-increment

    Number &operator--() { --x; --y; return *this; }          // pre-decrement
    Number operator--(int) { Number old = *this; --x; --y; return old; } // post-decrement

    void display(const char *label) const { cout << label << " = (" << x << ", " << y << ")\n"; }
};

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
