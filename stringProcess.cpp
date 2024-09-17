#include "stringProcess.h"
std::vector<std::string> split(std::string text, std::vector<std::string> delimiter/*separator,分隔符*/, std::string EscapeString /*char EscapeCharacter*/)
{
	std::vector<std::string> resultStrVec;
	for (std::vector<std::string>::iterator iter = delimiter.begin(); iter != delimiter.end(); iter++)
	{
		if ((iter + 1) != delimiter.end())
			text = join(split(text, *iter, EscapeString), *(iter + 1));
	}
	resultStrVec = split(text, delimiter.back(), EscapeString);
	return resultStrVec;
}
std::vector<std::string> split(std::string text, std::string delimiter/*separator,分隔符*/, std::string EscapeString /*char EscapeCharacter*/)
{
	text += delimiter;
	using namespace std;
	vector<string> resultStrVector;
	typedef long long llong;
	llong DelimiterCount = 0;
	string::size_type st1;
	string textProc = text;
	string textCut = text;
	string textTmpStorage;//临时储存转义的被剪裁的字符串
	while ((st1 = textCut.find(delimiter)) != string::npos)
	{//找到分隔符，接下来要判断分隔符前面是否有转义字符串
		string::size_type st2 = st1 - EscapeString.length();
		intptr_t aaa = st1 - EscapeString.length();
		if ((EscapeString != "") && (aaa < 0 ? false : (EscapeString == textCut.substr(st2, EscapeString.length()))))
		{//分隔符前面有转义字符串，接下来要判断转义字符串前面是否有转义字符串
			st2 = st1 - 2 * EscapeString.length();
			aaa = st1 - 2 * EscapeString.length();
			if (aaa < 0 ? false : (EscapeString == textCut.substr(st2, EscapeString.length())))
			{//转义字符串前面有转义字符串
				//去掉转义字符串，加入列表的字符串长度应该减小EscapeString.length()
				//先剪裁
				st1 -= EscapeString.length();//位置前移
				textCut = textCut.erase(st1 - EscapeString.length(), EscapeString.length());//删除转义字符串
				string textToAdd = (textTmpStorage == "" ? "" : textTmpStorage) + textCut.substr(0, st1);
				if (textToAdd != "")
					resultStrVector.push_back(textToAdd);//加入列表
				textTmpStorage = "";
				textCut = textCut.substr(st1 + delimiter.length());//剪裁字符串，去掉已经加入列表的部分
				//统计分隔符个数
				DelimiterCount += 1;
			}
			else {
				//先剪裁，去掉转义字符串
				//去掉转义字符串，加入列表的字符串长度应该减小EscapeString.length()
				st1 -= EscapeString.length();//位置前移
				textCut = textCut.erase(st1, EscapeString.length());//删除转义字符串
				textTmpStorage += textCut.substr(0, st1) + delimiter;//把转义字符串一并带上
				textCut = textCut.substr(st1 + delimiter.length());//剪裁字符串，去掉已经加入临时存储的字符串部分
				//统计分隔符个数，这里不是分隔符，所以不用DelimiterCount++;
			}
		}
		else {
			string textToAdd = (textTmpStorage == "" ? "" : textTmpStorage) + textCut.substr(0, st1);
			if (textToAdd != "")
				resultStrVector.push_back(textToAdd);
			textTmpStorage = "";
			textCut = textCut.substr(st1 + delimiter.length());//剪裁字符串，去掉已经加入列表的部分
			//统计分隔符个数
			DelimiterCount += 1;
		}
	}
	if (DelimiterCount == 0)
		resultStrVector.push_back(text);
	//"123456 789 4444 \ 55 \\ 11111 ";
	//123456;789;4444; 55;\;11111;
	return resultStrVector;
}
std::string join(std::vector<std::string> textVec, std::string delimiter)
{
	//using namespace std;
	using std::string;
	string resultString;
	for (string vecElement : textVec)
	{
		resultString += vecElement;
		resultString += delimiter;
	}
	resultString.pop_back();
	return resultString;
}

//std::vector<std::string> split(std::string text, std::vector<std::string> delimiter/*separator,分隔符*/, std::string EscapeString /*char EscapeCharacter*/)
//{
//	using namespace std;
//	int dlen = delimiter.size();
//	int tlen = text.length();
//	int elen = EscapeString.length();
//	//printf("1%s 1", delimiter[0]);
//	dlen = 1;
//	int firsti = 0;
//	int nums = 0;
//	std::vector<std::string> resultStrVector;
//	for (int i = 0; i < tlen; i++)
//	{
//
//		for (int k = 0; k < dlen; k++)
//		{
//			int flag1 = 0;
//			int delimiterlen = delimiter[k].length();
//			//printf("%d", delimiterlen);
//			for (int l = 0; l < delimiterlen; l++)
//			{
//				if (text[i + l] != delimiter[k][l])
//				{
//					flag1 = 1; break;
//				}
//			}
//			if (flag1 == 0)
//			{
//				resultStrVector.push_back(text.substr(firsti, i - firsti));
//				i += delimiterlen - 1;
//				firsti = i + 1;
//				break;
//			}
//
//		}
//		int flag = 0;
//		for (int k = 0; k < elen; k++)
//		{
//			if (text[i + k] != EscapeString[k])
//			{
//				flag = 1; break;
//			}
//		}
//		if (flag == 0)
//		{
//			for (int k = 0; k < dlen; k++)
//			{
//				flag = 0;
//				int delimiterlen = delimiter[k].length();
//				for (int l = 0; l < delimiterlen; l++)
//				{
//					if (text[i + elen + l] != delimiter[k][l])
//					{
//						flag = 1; break;
//					}
//				}
//				if (flag == 0)
//				{
//					i += delimiterlen - 1;
//					break;
//				}
//				for (int l = 0; l < elen; l++)
//				{
//					if (text[i + elen + l] != EscapeString[l])
//					{
//						flag = 1; break;
//					}
//				}
//				if (flag == 0)
//				{
//					i += elen - 1;
//					break;
//				}
//			}
//		}
//		//int count2;
//		//std::string* sss = new std::string[count2];
//	}
//	return resultStrVector;
//}
