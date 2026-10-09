#include <iostream>
using namespace std;
class Invoice
{
private:
string partNumber, partDescription;
int quantity;
double pricePerItem;
public:
Invoice(string pn, string pd, int q, double p)
{
partNumber = pn;
partDescription = pd;
if (q > 0)
quantity = q;
else
quantity = 0;
if (p > 0)
pricePerItem = p;
else
pricePerItem = 0.0;
}
void setQuantity(int q)
{
quantity = (q > 0) ? q : 0;
}
void setPrice(double p)
{
pricePerItem = (p > 0) ? p : 0.0;
}
string getPartNumber()
{
return partNumber;
}
string getPartDescription()
{
return partDescription;
}
int getQuantity()
{
return quantity;
}
double getPrice()
{
return pricePerItem;
}
double getInvoiceAmount()
{
return quantity * pricePerItem;
}
};
int main()
{
Invoice obj("P101", "Keyboard", 2, 500);
cout << "Part Number : " << obj.getPartNumber() << endl;
cout << "Description : " << obj.getPartDescription() << endl;
cout << "Quantity : " << obj.getQuantity() << endl;
cout << "Price Per Item : " << obj.getPrice() << endl;
cout << "Invoice Amount : " << obj.getInvoiceAmount() << endl;
return 0;
}