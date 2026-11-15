#include <iostream>
using namespace std;

// prints an amount rounded to 2 decimal places (e.g. 2006.666 -> 2006.67)
void printAmount(double amt) {
    long long paise = (long long)(amt * 100 + 0.5);
    cout << paise / 100 << '.';
    if (paise % 100 < 10) cout << '0';
    cout << paise % 100;
}

class SavingsAccount {
    static double annualInterestRate;  // as a fraction, e.g. 0.04
    double savingsBalance;
public:
    SavingsAccount(double balance) : savingsBalance(balance) {}

    void calculateMonthlyInterest() {
        savingsBalance += savingsBalance * annualInterestRate / 12;
    }
    static void modifyInterestRate(double rate) { annualInterestRate = rate; }
    double getBalance() const { return savingsBalance; }
};

double SavingsAccount::annualInterestRate = 0.0;

int main() {
    SavingsAccount saver1(2000.00), saver2(3000.00);

    SavingsAccount::modifyInterestRate(0.04);
    saver1.calculateMonthlyInterest();
    saver2.calculateMonthlyInterest();
    cout << "At 4%:\n  saver1 = Rs "; printAmount(saver1.getBalance());
    cout << "\n  saver2 = Rs "; printAmount(saver2.getBalance()); cout << endl;

    SavingsAccount::modifyInterestRate(0.05);
    saver1.calculateMonthlyInterest();
    saver2.calculateMonthlyInterest();
    cout << "At 5%:\n  saver1 = Rs "; printAmount(saver1.getBalance());
    cout << "\n  saver2 = Rs "; printAmount(saver2.getBalance()); cout << endl;
    return 0;
}
