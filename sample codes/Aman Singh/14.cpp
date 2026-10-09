#include <iostream>
using namespace std;

class Complex {
    int real, img;
public:
    Complex(int r = 0, int i = 0) : real(r), img(i) {}

    Complex operator+(const Complex &c) const { return Complex(real + c.real, img + c.img); }
    Complex operator-(const Complex &c) const { return Complex(real - c.real, img - c.img); }
    bool operator==(const Complex &c) const { return real == c.real && img == c.img; }

    void display() const {
        cout << real << (img < 0 ? " - " : " + ") << (img < 0 ? -img : img) << "i";
    }
};

int main() {
    int r, i;
    cout << "Enter real and imaginary part of first number: ";
    cin >> r >> i;
    Complex a(r, i);
    cout << "Enter real and imaginary part of second number: ";
    cin >> r >> i;
    Complex b(r, i);

    cout << "a = "; a.display(); cout << "\nb = "; b.display();
    cout << "\na + b = "; (a + b).display();
    cout << "\na - b = "; (a - b).display();
    cout << "\na == b : " << (a == b ? "true" : "false") << endl;
    return 0;
}
