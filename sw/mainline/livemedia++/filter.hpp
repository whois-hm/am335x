#pragma once

template <typename types, typename T>
class filter
{
	bool _bfiltering;
	T *_ptr;
public:
	T *ptr(){ return _ptr; }
	void enable()
	{ _bfiltering = true; }
	void disable()
	{ _bfiltering = false; }
	void operator << (types &pf)
	{ if(_bfiltering)operator >> (pf); }
	void operator << (types &&pf)
	{ if(_bfiltering)operator >> (pf); }

protected:
	filter(T *ptr) : _bfiltering(false) ,_ptr(ptr){ }
	virtual ~filter(){}
	virtual void operator >>(types &pf)
	{ }
};
