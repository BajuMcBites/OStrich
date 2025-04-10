void sort(int arr[], int l, int r) {
    if (l == r) return;
    int m = (l + r) >> 1;
    sort(arr, l, m);
    sort(arr, m + 1, r);
    int temp_arr[r - l + 1];
    int p1 = l;
    int p2 = m+1;
    for (int i = 0; i < r - l + 1; i++) {
        if (p1 <= m && (p2 > r || arr[p1] < arr[p2])) {
            temp_arr[i] = arr[p1++];
        }
        else {
            temp_arr[i] = arr[p2++];
        }
    }
    for (int i = 0; i < r - l + 1; i++) {
        arr[l + i] = temp_arr[i]; 
    }
}

int main() {
    const int n = 4;
    int arr[4] = {4, 6, 2, 10};
    sort(arr, 0, 3);
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i+1]) return 1;
    }
    return 0;
}