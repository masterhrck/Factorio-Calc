#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <string>

#define FACTORIO_VERSION "0.18.3"

using namespace std;
using json = nlohmann::json;
json j;
float speed = 0;
vector<string> suggests;

class Node {
public:
	string name;
	float rate;
	int level = 0;
	float assemblers = 0;
	vector<bool> activeL;
	bool isLast;
	Node(string n, float r) {
		name = n;
		rate = r;
	}
};

void err(string text) {
	cout << "Critical error: " << text << endl << endl;
	system("pause");
	exit(EXIT_FAILURE);
}

void printHeader(vector<Node>& nodes) {
	system("cls");
	cout << "-------- Selected --------" << endl << endl;
	for (Node node : nodes) {
		cout << "-> " << node.name << " (" << node.rate << "/m)" << endl << endl;
	}
	cout << "------- Speed: " << speed << " -------" << endl;
}

string printSuggestions(string query) {
	vector<string> names;
	for (auto it : j.items()) {
		if (it.key().find(query) != -1) {
			if (!it.value().is_null() && (it.value()["category"] == "crafting"))
				names.push_back(it.key());
		}
	}
	//Print
	if (names.size() == 0) {
		cout << "No suggestions found" << endl;
	}
	else if (names.size() == 1) {
		cout << "Using suggestion: " << names[0] << endl;
		return names[0];
	}
	else {
		cout << "Suggestions:" << endl;
		suggests.clear();
		for (int i = 0; i < names.size(); i++) {
			cout << "(" << i + 1 << ") " << names[i] << endl;
			suggests.push_back(names[i]);
		}
	}
	return "";
}

bool isNumber(const string &s) {
	string::const_iterator it = s.begin();
	while (it != s.end() && isdigit(*it)) ++it;
	return !s.empty() && it == s.end();
}

int main() {
	string lineIn;

	//Load json file to j
	fstream jfile;
	try {
		jfile.open("recipes.json");
	}
	catch (...) {
		err("Cannot find/open JSON file");
	}

	try {
		jfile >> j;
	}
	catch (...) {
		err("JSON parsing failed");
	}

	jfile.close();

	//Version info print
	cout << "Valid for Factorio v" << FACTORIO_VERSION << endl << endl;

	//Assembler type input
	cout << "Assembler type: " << endl
		<< "(1) Assembling machine 1" << endl
		<< "(2) Assembling machine 2" << endl
		<< "(3) Assembling machine 3" << endl;
	int assType;
	while (true) {
		assType = 0;
		cout << "> ";
		cin.clear();
		getline(cin, lineIn);
		try {
			assType = stoi(lineIn);
		}
		catch (...) {
			cout << "Error: not an int" << endl;
			continue;
		}
		if (assType < 1 || assType > 3) {
			cout << "Error: invalid choice" << endl;
			continue;
		}
		else {
			break;
		}
	}

	//Speed bonus input
	int speedBonus = -1;
	do {
		cout << endl << "Speed bonus (%): ";
		cin.clear();
		getline(cin, lineIn);
		if (lineIn != "") {
			try { speedBonus = stoi(lineIn); }
			catch (...) {
				cout << "Error: not an int" << endl;
			}
		}
		else {
			speedBonus = 0;
		}

	} while (speedBonus == -1);

	switch (assType) {
	case 1:
		speed = 0.5;
		break;
	case 2:
		speed = 0.75;
		break;
	case 3:
		speed = 1.25;
		break;
	}

	speed = speed * (100 + speedBonus) / 100;

	//Nodes input
	vector<Node> nodes;
	printHeader(nodes);
	while (true) {
		cout << endl << "Item name [and rate]: ";
		getline(cin, lineIn);
		if (lineIn == "") {
			if (nodes.size() != 0)
				break;
			continue;
		}

		string name = lineIn.substr(0, lineIn.find_first_of(' '));
		if (j[name].is_null() || !(j[name]["category"]=="crafting")) {
			if (isNumber(name)) {
				int choice = stoi(name);
				if (choice < 1 || choice > suggests.size()) {
					if (suggests.size() == 0)
						cout << "Error: no choices available" << endl;
					else
						cout << "Error: invalid choice" << endl;
					continue;
				}
				else {
					name = suggests[choice - 1];
					cout << "(" << choice << ") " << name << endl;
				}
			}
			else {
				cout << "Item not in database" << endl;
				name = printSuggestions(name);
			}
			//If Failure to get good name
			if(name=="")
				continue;
		}
		//Second chance for input of rate (if not specified in the same line as name)
		if (lineIn.find_first_of(' ') == -1) {
			string lineIn2;
			cout << "Rate: ";
			getline(cin, lineIn2);
			lineIn += " ";
			lineIn += lineIn2;
		}
		float rate;
		try {
			rate = stof(lineIn.substr(lineIn.find_first_of(' '), lineIn.length()));
		}
		catch (...) {
			cout << "Error: not a float" << endl;
			continue;
		}

		nodes.push_back(Node(name, rate));
		printHeader(nodes);
		suggests.clear();
	}

	vector<Node> displayNodes = nodes;

	//Searching for dependencies, inserting to array
	for (unsigned int i = 0; i < nodes.size(); i++) {
		json result = j[nodes[i].name];
		if (!result.is_null() && (result["category"] == "crafting")) {
			float time = result["time"];
			int nOut = result["products"][0]["amount"];
			for (json ingr : result["ingredients"]) {
				int n = ingr["amount"];
				string name = ingr["name"];
				float rate = (nodes[i].rate * n) / nOut;
				Node node = Node(name, rate);
				node.level = nodes[i].level + 1;
				nodes.insert(nodes.begin() + i + 1, node);
			}
			nodes[i].assemblers = (nodes[i].rate * time) / (nOut * speed * 60);
		}
	}

	//Format prepare
	int maxLevel = 0;
	for (Node node : nodes) {
		maxLevel = max(maxLevel, node.level);
	}
	//Iterating from end to beginning
	vector<bool> lastActiveL(maxLevel + 1, false);
	for (unsigned int i = nodes.size() - 1; i > 0; i--) {
		nodes[i].activeL = lastActiveL;
		if (nodes[i].activeL[nodes[i].level] == false)
			nodes[i].isLast = true;
		nodes[i].activeL[nodes[i].level] = true;
		for (int lvl = nodes[i].level + 1; lvl < nodes[i].activeL.size(); lvl++) {
			nodes[i].activeL[lvl] = false;
		}
		lastActiveL = nodes[i].activeL;
	}

	//Printing to array
	vector<string> output;
	for (unsigned int i = 0; i < nodes.size(); i++) {
		int indents = max(0, nodes[i].level * 4 - 3);
		string lineB(indents, ' ');

		for (int lvl = 1; lvl < nodes[i].activeL.size(); lvl++) {
			int pos = max(0, (lvl - 1) * 4);
			if (nodes[i].activeL[lvl]) {
				lineB[pos] = '|';
			}
		}

		string lineA = lineB;
		if (nodes[i].level > 0) {
			int pos = max(0, (nodes[i].level - 1) * 4);
			if (nodes[i].isLast)
				lineB[pos] = '\\';
			else
				lineB[pos] = '+';

			lineB += "---";
		}
		lineB += nodes[i].name;

		if (nodes[i].assemblers != 0) {
			lineB += " [";
			string niceAss = to_string(round(nodes[i].assemblers * 10) / 10);
			niceAss = niceAss.substr(0, niceAss.find(".") + 2);
			lineB += niceAss;
			lineB += "x]";
		}

		lineB += " (";
		string niceRate = to_string(round(nodes[i].rate * 10) / 10);
		niceRate = niceRate.substr(0, niceRate.find(".") + 2);
		lineB += niceRate;
		lineB += "/m)";

		if (nodes[i].level == 0)
			output.push_back("");
		output.push_back(lineA);
		output.push_back(lineB);
	}

	//Printing
	printHeader(displayNodes);
	for (string line : output) {
		cout << "  " << line << endl;
	}
	cout << endl << endl;
	system("pause");

	return 0;
}
