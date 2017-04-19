/*************************************************************************
    > File Name: comm_struct.h
    > Author: ma6174
    > Mail: ma6174@163.com 
    > Created Time: Sat 25 Apr 2015 12:43:14 AM PDT
 ************************************************************************/
#ifndef COMM_STRUCT_H
#define COMM_STRUCT_H

#include<iostream>
#include <sstream>
#include <map>
using namespace std;
/***
//在函数中不能使用某些内部成员，比如iterator, 暂且放弃模板化此函数。
template<class K, class V> string map2str(const map<K, V>& m)
{
	ostringstream ss;
	ss << "map {";
	std::map<K, V>::const_iterator it=m.begin();
	for(; it!=m.end(); it++){
		ss<< endl<< "\""<< it->first<< "\": \""<< it->second<< "\"; ";
	}
	ss <<"}";
	return ss.str();
}
*/
string map2str(const map<char, string>& m)
{
	ostringstream ss;
	ss << "map {";
	std::map<char, string>::const_iterator it=m.begin();
	for(; it!=m.end(); it++){
		ss<< endl<< "\""<< it->first<< "\": \""<< it->second<< "\"; ";
	}
	ss <<"}";
	return ss.str();
}
#endif
