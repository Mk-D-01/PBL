#include <iostream>
#include <iomanip>
using namespace std;
class Time
{
private:
 int hours, minutes, seconds;
public:
 Time()
 {
 hours = 0;
 minutes = 0;
 seconds = 0;
 }
 Time(int h, int m, int s)
 {
 hours = h;
 minutes = m;
 seconds = s;
 }
 void display()
 {
 cout << setfill('0') << setw(2) << hours << ":"
 << setw(2) << minutes << ":"
 << setw(2) << seconds << endl;
 }
 void add(Time t1, Time t2)
 {
 seconds = t1.seconds + t2.seconds;
 minutes = t1.minutes + t2.minutes;
 hours = t1.hours + t2.hours;
 if (seconds >= 60)
 {
 seconds -= 60;
 minutes++;
 }
 if (minutes >= 60)
 {
 minutes -= 60;
 hours++;
 }
 }
};
int main()
{
 Time time1(10, 20, 30);
 Time time2(2, 45, 45);
 Time time3;
 time3.add(time1, time2);
 cout << "Time 1: ";
 time1.display();
 cout << "Time 2: ";
 time2.display();
 cout << "Sum : ";
 time3.display();
 return 0;
}