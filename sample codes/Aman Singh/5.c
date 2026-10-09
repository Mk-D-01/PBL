#include <iostream>
using namespace std;

class library {
    string book_name;
    int date;

public:
    void input() {
        cout << "Enter name of the book you want to issue : ";
        cin >> book_name;
        cout << "Enter the date till you want the book : ";
        cin >> date;
    }

    void issue() {
        cout << book_name << " Book Issued." << endl;
        cout << "Till : " << date << endl;
    }
};

int main() {
    library L1;
    L1.input();
    L1.issue();

    return 0;
}