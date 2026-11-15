#include <iostream>
using namespace std;

class Hotel {
    int Rno;
    string name;
    int tariff;
    int NOD;

public:
    void checkin() {
        cout << "Enter Your Room no : ";
        cin >> Rno;
        cout << "Enter Your Name : ";
        cin >> name;
        cout << "Enter Your Charges : ";
        cin >> tariff;
        cout << "Enter Your days to stay : ";
        cin >> NOD;
    }

    void checkout() {
        cout << Rno << endl;
        cout << name << endl;
        cout << tariff << endl;
        cout << NOD << endl;
        CALC();
    }

private:
    void CALC() {
        int amount = 0;
        amount = NOD * tariff;

        if (amount > 10000) {
            amount = amount * 1.5;
            cout << "Amount to be paid : " << amount;
        } else {
            cout << "Amount to be paid : " << amount;
        }
    }
};

int main() {
    Hotel H1;
    H1.checkin();
    H1.checkout();

    return 0;
}