#ifndef __STRINGUTIL_HXX
#define __STRINGUTIL_HXX

#include <string>
#include <vector>

class StringVector {
public:
	explicit StringVector(const std::string& inStr);
	int size() const;
	bool empty() const;
	std::string at(int index) const;
	const char* atPtr(int index) const;
private:
	struct SubStringIndex {
		std::string::size_type offset;
		std::string::size_type length;
	};
	std::vector<SubStringIndex> mIndexes;
	std::string mString;
};

#endif /* __STRINGUTIL_HXX */