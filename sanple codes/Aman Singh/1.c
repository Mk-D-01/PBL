#include <iostream>
using namespace std;

int main() {
    int unit;
    cout << "Enter the units used : ";
    cin >> unit;

    string name;
    cout << "Enter your name : ";
    cin >> name;

    int total = 0, final_total = 0;

    if (unit <= 100) {
        total = unit * 0.6;
        cout << name << endl;
        cout << "Your Total bill is : " << total;
    } 
    else if (unit > 100 && unit <= 300) {
        unit -= 100;
        total = (unit * 0.8) + 60;
        cout << name << endl;
        cout << "Your Total bill is : " << total;
    } 
    else if (unit > 300) {
        unit -= 300;
        total = (unit * 0.9) + 220;
        final_total = 50 + (total * 0.15);
        cout << name << endl;
        cout << "Your Total bill is : " << total;
    }

    return 0;
}