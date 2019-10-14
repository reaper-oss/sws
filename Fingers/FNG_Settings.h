#ifndef _FNG_SETTINGS_H_
#define _FNG_SETTINGS_H_

template <typename T>
inline void setReaperProperty(const std::string &key, const T& value)
{
	std::ostringstream oss;
	oss << value;
	WritePrivateProfileString("fingers", key.c_str(), oss.str().c_str(), get_ini_file());
}

template <>
inline void setReaperProperty<std::string>(const std::string &key, const std::string &value)
{
	WritePrivateProfileString("fingers", key.c_str(), value.c_str(), get_ini_file());
}

inline std::string getReaperProperty(const std::string &key)
{
	char value[512];
	GetPrivateProfileString("fingers", key.c_str(), NULL, value, 512, get_ini_file());
	return std::string(value);
}

#endif /*_FNG_SETTINGS_H_ */
