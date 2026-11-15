#include<iostream>
using namespace std;
class area
{
public:
void calculatearea(int side)
{
cout<<"Area of square= "<<side *side<<endl;
}
void calculatearea(int length, int breadth)
{
cout<<"area of rectangle= "<<length*breadth<<endl;
}
void calculatearea(float base , float height)
{
cout<<"Area of triangle = "<<0.5*base*height<<endl;
}
};
int main()
{
area obj;
obj.calculatearea(4);
obj.calculatearea(5,6);
obj.calculatearea(4.0f,5.0f);
return 0;
}