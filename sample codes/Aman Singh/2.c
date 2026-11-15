#include <iostream>
#include <string>
using namespace std;

int main() {
    string str;
    char ch;

    cout << "Enter a string: ";
    getline(cin, str);

    cout << "Enter the character to remove: ";
    cin >> ch;

    string result = "";

    for (int i = 0; i < str.length(); i++) {
        char c = str[i];
        if (c != ch) {
            result += c;
        }
    }

    cout << "Updated string: " << result << endl;

    return 0;
}