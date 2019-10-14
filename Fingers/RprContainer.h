#ifndef __RPR_CONTAINER_HXX
#define __RPR_CONTAINER_HXX

template
<typename T>
class RprContainer {
public:
	RprContainer() {}

	int size()
	{
		return (int)mItems.size();
	}
	
	void add(const T &item)
	{
		mItems.push_back(item);
	}

	void remove(const T &item)
	{
		for(typename std::vector<T>::iterator i = mItems.begin(); i != mItems.end(); i++) {
			if(i->toReaper() == item.toReaper()) {
				mItems.erase(i);
				return;
			}
		}
	}

	void removeAt(int index)
	{
		mItems.erase(mItems.begin() + index);
	}

	T getAt(int index)
	{
		return *(mItems.begin() + index);
	}

	T first() { return getAt(0); }
	T last() { return getAt(size() - 1); }

	void sort()
	{
		doSort();
	}
	
	virtual ~RprContainer() {}

protected:
	virtual void doSort() = 0;
	std::vector<T> mItems;
};

#endif /* __RPR_CONTAINER_HXX */
