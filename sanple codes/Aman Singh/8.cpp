#include<iostream>
using namespace std;
class Student{
int scores[5];
public:
void input(){
for(int i=0;i<5;i++)
cin>>scores[i];
}
int calculateTotalScore(){
int sum=0;
for(int i=0;i<5;i++)
sum=sum+scores[i];
return sum;
}
};
int main(){
int n,count=0;
cout<<"Enter number of students: ";
cin>>n;
Student s[n];
cout<<"Enter 5 marks of each student:"<<endl;
for(int i=0;i<n;i++)
s[i].input();
int anna=s[0].calculateTotalScore();
for(int i=1;i<n;i++){
if(s[i].calculateTotalScore()>anna)
count++;
}
cout<<"\nStudents scoring higher than Anna: "<<count;
return 0;
}