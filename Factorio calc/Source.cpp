#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <string>

using namespace std;
using json = nlohmann::json;
json j;
float speed = 0;

constexpr float defaultSpeed = 0.5;

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
	cout << "Error: " << text << endl << endl;
	system("pause");
	exit(EXIT_FAILURE);
}

void printHeader(vector<Node>& nodes) {
	system("cls");
	cout << "-------- Selected --------" << endl << endl;
	for (auto node : nodes) {
		cout << "-> " << node.name << " (" << node.rate << "/m)" << endl << endl;
	}
	cout << "------- Speed: " << speed << " -------" << endl;
}

string printSuggestions(string query) {
	vector<string> names;
	for (auto it : j.items()) {
		if (it.key().find(query) != -1) {
			if (!it.value().is_null())
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
		for (auto name : names) {
			cout << "  " << name << endl;
		}
	}
	return "";
}

int main() {
	string lineIn;

	//Load json file to j
	fstream jfile;
	try {
		jfile.open("recipes.json");
	}
	catch (exception e) {
		err("Cannot find JSON file");
	}
	try {
		jfile >> j;
	}
	catch (exception e) {
		err("JSON parsing failed");
	}
	jfile.close();

	//Assembler type input
	cout << "Assembler type: " << endl
		<< "(1) Assembling machine 1" << endl
		<< "(2) Assembling machine 2" << endl
		<< "(3) Assembling machine 3" << endl
		<< "> ";
	int assType = 0;
	while (assType < 1 || assType > 3) {
		cin >> assType;
	}

	//Speed input
	int speedBonus = -1;
	do {
		cout << endl << "Speed bonus (%): ";
		cin.ignore();
		getline(cin, lineIn);
		if (lineIn != "") {
			try { speedBonus = stoi(lineIn); }
			catch (exception e) {
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
		cout << endl << "Node: ";
		getline(cin, lineIn);
		if (lineIn == "") {
			if (nodes.size() != 0)
				break;
			continue;
		}

		string name = lineIn.substr(0, lineIn.find_first_of(' '));
		if (j[name].is_null()) {

			cout << "Item not in database" << endl;
			name = printSuggestions(name);
			if(name=="")
				continue;
		}
		//Rate second chance
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
		catch (exception e) {
			cout << "Error: not a float" << endl;
			continue;
		}

		nodes.push_back(Node(name, rate));
		printHeader(nodes);
	}

	auto displayNodes = nodes;

	//Searching for dependencies, inserting to array
	for (unsigned int i = 0; i < nodes.size(); i++) {
		json res = j[nodes[i].name];
		if (!res.is_null()) {
			float time = res["time"];
			int nOut = res["products"][0]["amount"];
			for (json ingr : res["ingredients"]) {
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
