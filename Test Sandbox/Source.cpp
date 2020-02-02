#include <iostream>
#include <string>

using namespace std;

int main() {
	string text;
	do {
		getline(cin, text);
		cout << text << endl;
	} while (text != "");

	return 0;
}