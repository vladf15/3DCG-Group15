#include"Grammar.h"
Grammar::Grammar(std::string ax, std::vector <std::tuple<char, std::string>> rls, int n) {
	axiom = ax;
	rules = rls;
	steps = {};
	steps.push_back(axiom);
	for (int i = 1; i < n; i++) {
		std::string new_s = "";
		for (int j = 0; j < steps[i - 1].length(); j++) {
			bool added = false;
			for (int k = 0; k < rules.size(); k++) {
				if (std::get<0>(rules[k]) == steps[i - 1][j]) {
					added = true;
					new_s += std::get<1>(rules[k]);
				}
			}
			if (!added) {
				new_s += steps[i - 1][j];
			}
		}
		steps.push_back(new_s);
	}
	PrintInfo(true);
}
std::string Grammar::PrintInfo(bool iter) {
	std::string s = "";
	s += "Axiom: " + axiom + '\n';
	s += "Rules: \n";
	for (int i = 0; i < rules.size(); i++) {
		s += std::string(1, std::get<0>(rules[i])) + " -> " + std::get<1>(rules[i]) + '\n';
	}
	if (iter) {
		s += "Iterations: \n";
		for (int i = 0; i < steps.size(); i++) {
			s += "Iteration " + std::to_string(i) + ": \n" + steps[i] + '\n';
		}

		std::cout << "______________________________" << std::endl;
		std::cout << s << std::endl;
		std::cout << "______________________________" << std::endl;
	}
	return s;
}