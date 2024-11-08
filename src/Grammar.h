#pragma once
#ifndef GRAMMAR_CLASS_H
#define GRAMMAR_CLASS_H
#include<string>
#include<vector>
#include<iostream>
#include<tuple>
class Grammar {
public:
	std::string axiom;
	std::vector <std::tuple<char, std::string>> rules;
	std::vector<std::string> steps;
	Grammar(std::string ax, std::vector <std::tuple<char, std::string>> rls, int n);
	std::string PrintInfo(bool iter);
};
#endif