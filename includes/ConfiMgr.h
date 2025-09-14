#ifndef CONFIMGR_H
#define CONFIMGR_H
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <map>
#include <string>

struct SectionInfo
{
	SectionInfo(){}
	~SectionInfo()
	{

	}
	SectionInfo(const SectionInfo& other)
	{
		section_datas = other.section_datas;
	}

	SectionInfo& operator=(const SectionInfo& other)
	{
		if (this != &other)
		{
			section_datas = other.section_datas;
		}
		return *this;
	}
	std::map<std::string, std::string>section_datas;
	std::string operator[](const std::string& key)
	{
		if(section_datas.find(key)== section_datas.end())
		{
			return "";
		}
		return section_datas[key];
	}
};
class ConfiMgr
{
	
public:
	~ConfiMgr() {
		config_map.clear();
	}

	static ConfiMgr& getInstance() 
	{
		static ConfiMgr instance;
		return instance;
	}

	SectionInfo operator[](const std::string& section)
	{
		if (config_map.find(section) == config_map.end())
		{
			return SectionInfo();
		}
		return config_map[section];
	}

	ConfiMgr& operator=(const ConfiMgr& other)
	{
		if (this != &other)
		{
			config_map = other.config_map;
		}
		return *this;
	}

	ConfiMgr(const ConfiMgr& other) 
	{
		config_map = other.config_map;
	}
private:
	std::map<std::string, SectionInfo>config_map; 
	ConfiMgr();
};
#endif // CONFIMGR_H

// Implementation of the private default constructor
