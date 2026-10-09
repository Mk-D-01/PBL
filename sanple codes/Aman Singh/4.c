#include <iostream>
using namespace std;

int main() {
    int n;
    cout << "Enter the number of elements : ";
    cin >> n;

    int arr[1000];

    cout << "Enter " << n << " elements: ";
    for (int i = 0; i < n; i++) {
        cin >> arr[i];
    }

    int max_sum = 0;

    for (int i = 0; i < n; i++) {
        int crt_sum = 0;
        int crt_idx = 1;
        int grp_size = i;

        while (crt_idx + grp_size <= n) {
            for (int j = 0; j < grp_size; j++) {
                crt_sum += arr[j];
                crt_idx++;
            }
            grp_size++;
        }

        if (crt_sum > max_sum) {
            max_sum = crt_sum;
        }
    }

    cout << "Maximum Special Sum = " << max_sum << endl;

    return 0;
}