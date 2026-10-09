#include <iostream>
#include <string>
using namespace std;

int main() {
    string str;

    cout << "Enter a string: ";
    getline(cin, str);

    cout << "Non-repeating characters: ";

    for (int i = 0; i < str.length(); i++) {
        int count = 0;

        for (int j = 0; j < str.length(); j++) {
            if (str[i] == str[j] && str[i] != ' ') {
                count++;
            }
        }

        if (count == 1) {
            cout << str[i] << " ";
        }
    }

    return 0;
}