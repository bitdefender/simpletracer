#ifndef __BITMAP__
#define __BITMAP__
class BitMap {
private :
	unsigned int sz;
	unsigned int ct;
	unsigned int rw;
	bool isZero;

	unsigned int refCount;

	std::vector<unsigned int> data;

	inline void Init(unsigned int size, unsigned int rows);
public :
	static int instCount;

	static BitMap *Union(const BitMap &b1, const BitMap &b2);
	void Union(const BitMap &rhs);

	BitMap(unsigned int size, unsigned int rows);
	BitMap(const BitMap &b1, const BitMap &b2);
	BitMap(const BitMap &o, unsigned int c = 1);
	BitMap(const BitMap &o, unsigned int s, unsigned int c);

	void SetBit(unsigned int p);
	bool GetBit(unsigned int p) const;

	bool IsZero() const;

	void Print() const;

	void AddRef();
	void DelRef();
};

#endif
