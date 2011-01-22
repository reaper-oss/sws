#ifndef _FNG_SETTINGS_H_
#define _FNG_SETTINGS_H_

template
<typename T>
__inline void setReaperProperty(const std::string &key, const T& value)
{
	std::ostringstream oss;
	oss << value;
	WritePrivateProfileString("fingers", key.c_str(), oss.str().c_str(), get_ini_file());
}

template <>
__inline void setReaperProperty<std::string>(const std::string &key, const std::string &value)
{
	WritePrivateProfileString("fingers", key.c_str(), value.c_str(), get_ini_file());
}

__inline std::string getReaperProperty(const std::string &key)
{
	char value[512];
	GetPrivateProfileString("fingers", key.c_str(), NULL, value, 512, get_ini_file());
	return std::string(value);
}

__inline void *getInternalReaperProperty(const std::string &key)
{
	int temp;
	int offset = projectconfig_var_getoffs(key.c_str(), &temp);
	if (offset)
		return projectconfig_var_addr(0, offset);
	else
		return get_config_var(key.c_str(), &offset);
}

#endif /*_FNG_SETTINGS_H_ */