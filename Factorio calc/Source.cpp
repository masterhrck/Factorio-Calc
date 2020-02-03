#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <string>

using namespace std;
using json = nlohmann::json;
json j;

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
	cout << "Error: " << text << endl;
	string line;
	getline(cin, line);
	exit(EXIT_FAILURE);
}

int main() {
	string lineIn;

	//Load json file to j
	fstream jfile;
	try {
		jfile.open("recipes.json");
	}
	catch (exception e) {
		err("Cannot open JSON file");
	}
	try {
		jfile >> j;
	}
	catch (exception e) {
		err("JSON parsing failed");
	}
	jfile.close();

	//Speed input
	float speed = 0;
	do {
		cout << "Speed: ";
		getline(cin, lineIn);
		if (lineIn == "") {
			speed = defaultSpeed;
			cout << "Using default speed: " << defaultSpeed << endl << endl;
		}
		else {
			try { speed = stof(lineIn); }
			catch (exception e) {
				cout << "Error: not a float" << endl;
			}
		}
	} while (speed == 0);


	//Nodes input
	vector<Node> nodes;
	while (true) {
		cout << "Node: ";
		getline(cin, lineIn);
		if (lineIn == "") {
			if (nodes.size() != 0)
				break;
			continue;
		}
		
		string name = lineIn.substr(0, lineIn.find_first_of(' '));
		if (j[name].is_null()) {
			cout << "Error: item not in database" << endl;
			continue;
		}
		if (lineIn.find_first_of(' ')==-1) {
			string lineIn2;
			cout << "Enter rate for " << name << ": ";
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
		cout << endl;
	}

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

	//Print
	vector<string> output;
	for (unsigned int i = 0; i < nodes.size(); i++) {
		int indents = max(0, nodes[i].level * 4 - 3);
		string lineB(indents, ' ');

		for (int lvl = 1; lvl < nodes[i].activeL.size(); lvl++) {
			int pos = max(0, (lvl-1) * 4);
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

		output.push_back(lineA);
		output.push_back(lineB);
	}
	int maxLineLen = 0;
	for (string line : output) {
		maxLineLen = max(maxLineLen, (int)line.size());
	}
	string border(maxLineLen, '=');
	cout << endl << border << endl;
	for (string line : output) {
		cout << line << endl;
	}
	cout << endl << border << endl;
	getline(cin, lineIn);

	return 0;
}