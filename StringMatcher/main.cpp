#include <iostream>
#include <chrono>
#include <regex>
#include <unordered_map>
#include <cstdint>
#include "CuStringMatcher.h"

int64_t GetTimeStampMs()
{
	auto time_pt = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
	return time_pt.time_since_epoch().count();
}

int main()
{
	using namespace CU;
	{
		std::cout << "Test Match Front:" << std::endl;
		StringMatcher matcher("Hello*");
		std::cout << (matcher.match("Hello, World!") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("World, Hello!") ? "true" : "false") << std::endl;
	}
	{
		std::cout << "Test Match Middle:" << std::endl;
		StringMatcher matcher("*lo, Wo*");
		std::cout << (matcher.match("Hello, World!") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("World, Hello!") ? "true" : "false") << std::endl;
	}
	{
		std::cout << "Test Match Back:" << std::endl;
		StringMatcher matcher("*World!");
		std::cout << (matcher.match("Hello, World!") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("World, Hello!") ? "true" : "false") << std::endl;
	}
	{
		std::cout << "Test Match Entire:" << std::endl;
		StringMatcher matcher("Hello, World!");
		std::cout << (matcher.match("Hello, World!") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("Hello,World!") ? "true" : "false") << std::endl;
	}
	{
		std::cout << "Test Multi Key:" << std::endl;
		StringMatcher matcher("(test|Hello)*");
		std::cout << (matcher.match("Hello, World!") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("test, World!") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("World, Hello!") ? "true" : "false") << std::endl;
	}
	{
		std::cout << "Test Multi Rule:" << std::endl;
		StringMatcher matcher("*World!|test*");
		std::cout << (matcher.match("Hello, World!") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("test, hello!") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("Hello, test!") ? "true" : "false") << std::endl;
	}
	{
		std::cout << "Test charSet:" << std::endl;
		StringMatcher matcher("[A-Z][0-9]");
		std::cout << (matcher.match("A0") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("B1") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("a9") ? "true" : "false") << std::endl;
	}
	{
		std::cout << "Test complex string:" << std::endl;
		StringMatcher matcher("[Hh]ello, [Ii]'m [0-9][0-9] years old, my favorite letter is [A-Z].");
		std::cout << (matcher.match("Hello, I'm 18 years old, my favorite letter is G.") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("Hello, I'm 24 years old, my favorite letter is A.") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("hello, i'm 99 years old, my favorite letter is Z.") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("Hello, I'm 18 years old, my favorite letter is z.") ? "true" : "false") << std::endl;
		std::cout << (matcher.match("Hello, I'm 1 years old, my favorite letter is F.") ? "true" : "false") << std::endl;
	}
	{
		std::cout << "Test setRule & clear:" << std::endl;

		StringMatcher matcher{};
		matcher.setRule("*Hello*");
		std::cout << (matcher.match("Hello India Mi Fans, do you like mi 4i?") ? "true" : "false") << std::endl;
		matcher.clear();
		std::cout << (matcher.match("Hello India Mi Fans, do you like mi 4i?") ? "true" : "false") << std::endl;
		std::cout << matcher.rule() << std::endl;
	}
	{
		std::cout << "std::vector & std::unordered_map support test." << std::endl;

		StringMatcher matcher = StringMatcher("*(this|test)*");
		std::cout << (matcher.match("this is test text") ? "true" : "false") << std::endl;

		std::vector<StringMatcher> testList{};
		testList.resize(100, StringMatcher("Hello*"));
		std::cout << testList[0].rule() << std::endl;

		std::unordered_map<int, StringMatcher> testMap{};
		testMap[123] = StringMatcher("Hello*");
		testMap[123] = StringMatcher();
		std::cout << testMap.at(123).rule() << std::endl;
	}

	std::cout << "Speed Test:" << std::endl;
	{
		auto startTime = GetTimeStampMs();
		for (int i = 0; i < 10000; i++) {
			std::regex testRegex("^(Red|Orange|Yello|Green|Blue|Purple|White|Black|Grey|Gold|Silver)");
		}
		std::cout << "std::regex use time: " << GetTimeStampMs() - startTime << " ms." << std::endl;
	}
	{
		auto startTime = GetTimeStampMs();
		for (int i = 0; i < 10000; i++) {
			StringMatcher testMatch("(Red|Orange|Yello|Green|Blue|Purple|White|Black|Grey|Gold|Silver)*");
		}
		std::cout << "StringMatcher use time: " << GetTimeStampMs() - startTime << " ms." << std::endl;
	}
	{
		std::regex testRegex("^The price of the shirt is [0-9] pounds");
		auto startTime = GetTimeStampMs();
		for (int i = 0; i < 100000; i++) {
			bool match = false;
			match = std::regex_search("The price of the shirt is 9 pounds 15 pence.", testRegex);
			match = std::regex_search("The price of the skirt is 1 pounds 99 pence.", testRegex);
			match = std::regex_search("The shirt is free.", testRegex);
		}
		std::cout << "std::regex_search use time: " << GetTimeStampMs() - startTime << " ms." << std::endl;
	}
	{
		StringMatcher testMatch("The price of the shirt is [0-9] pounds*");
		auto startTime = GetTimeStampMs();
		for (int i = 0; i < 100000; i++) {
			bool match = false;
			match = testMatch.match("The price of the shirt is 9 pounds 15 pence.");
			match = testMatch.match("The price of the skirt is 1 pounds 99 pence.");
			match = testMatch.match("The shirt is free.");
		}
		std::cout << "StringMatcher use time: " << GetTimeStampMs() - startTime << " ms." << std::endl;
	}

	return 0;
}
