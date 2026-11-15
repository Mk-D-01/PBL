#include <iostream>
using namespace std;

class bankAccount {
    string name;
    int acc_no;
    string acc_type;
    float bal_amt;

public:
    void input() {
        cout << "Enter your name : ";
        cin >> name;
        cout << "Enter your account number : ";
        cin >> acc_no;
        cout << "Enter your account type : ";
        cin >> acc_type;
        cout << "Enter your balance amount in account : ";
        cin >> bal_amt;
        display();
    }

    void deposit() {
        int dep_osit;
        cout << "Enter the amount you want to deposit : ";
        cin >> dep_osit;
        if (dep_osit > 0) {
            bal_amt += dep_osit;
            display();
        } else {
            cout << "Enter a Valid Amount";
        }
    }

    void withdraw() {
        int with_draw;
        cout << "Enter the amount you want to withdraw : ";
        cin >> with_draw;
        if (bal_amt > 0 && bal_amt >= with_draw) {
            bal_amt -= with_draw;
            display();
        } else {
            cout << "Your balance is too low";
        }
    }

private:
    void display() {
        cout << name << endl;
        cout << bal_amt << endl;
    }
};

int main() {
    bankAccount U1;
    U1.input();
    U1.deposit();
    U1.withdraw();

    return 0;
}