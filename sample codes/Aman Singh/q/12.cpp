#include <iostream>
using namespace std;

// prints a number with at least two digits (e.g. 5 -> 05)
void print2(int n) {
    if (n < 10) cout << '0';
    cout << n;
}

class Time {
    int hours, minutes, seconds;
public:
    Time() : hours(0), minutes(0), seconds(0) {}
    Time(int h, int m, int s) : hours(h), minutes(m), seconds(s) {}

    void display() const {
        print2(hours); cout << ':';
        print2(minutes); cout << ':';
        print2(seconds); cout << endl;
    }

    // Sets *this = a + b, normalising seconds/minutes overflow
    void add(const Time &a, const Time &b) {
        int total = (a.hours + b.hours) * 3600 + (a.minutes + b.minutes) * 60
                    + a.seconds + b.seconds;
        hours = total / 3600;
        minutes = (total % 3600) / 60;
        seconds = total % 60;
    }
};

int main() {
    Time t1(11, 59, 59), t2(1, 0, 1), t3;
    t3.add(t1, t2);
    cout << "t1 = "; t1.display();
    cout << "t2 = "; t2.display();
    cout << "t3 = t1 + t2 = "; t3.display();
    return 0;
}
