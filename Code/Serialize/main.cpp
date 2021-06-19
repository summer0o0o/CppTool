// Serialize.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "TestJson.h"
#include "JsonSerialize.hpp"


int main()
{
	//TestJson::JsonBase();
	TestJson::JsonFifo();

	JsonSerialize<unordered_json> js(L"./test2.json");

	int age = 18;
	std::wstring name = L"Will";
	std::vector<std::string> names = { "Will", "Nance" };
	//std::pair<int, std::string> index_name(1, "Will");

	js.SetParam("name", name);
	js.SetParam("dge", 133);
	js.SetParam("age", age);
	js.SetParam("list", names);

	js.SaveCfg();
	js.ConsolePrint();
	js.LoadCfg();
	js.GetParam("age", age);
	js.GetParam("name", name);
	js.GetParam("list", names);

	return 0;
}
