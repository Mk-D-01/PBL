#include <iostream>
#include <conio.h>
using namespace std;
class TollBooth
{
private:
 unsigned int totalCars;
 double totalCash;
public:
 TollBooth()
 {
 totalCars = 0;
 totalCash = 0;
 }
 void payingCar()
 {
 totalCars++;
 totalCash += 0.5;
 }
 void nonPayCar()
 {
 totalCars++;
 }
 void display()
 {
 cout << "\nTotal cars: " << totalCars;
 cout << "\nTotal cash: $" << totalCash << endl;
 }
};
int main()
{
 TollBooth booth;
 char ch;
 cout << "Press P for paying car";
 cout << "\nPress N for non-paying car";
 cout << "\nPress ESC to exit\n";
 while (true)
 {
 ch = _getch();
 if (ch == 'P' || ch == 'p')
 booth.payingCar();
 else if (ch == 'N' || ch == 'n')
 booth.nonPayCar();
 else if (ch == 27)
 {
 booth.display();
 break;
 }
 }
 return 0;
}