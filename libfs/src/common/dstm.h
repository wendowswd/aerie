#ifndef _TRANSACTION_STAMNOS_H_BMA567
#define _TRANSACTION_STAMNOS_H_BMA567

#include <google/sparsehash/sparseconfig.h>
#include <google/dense_hash_map>
#include "common/types.h"

// Currently we support just read-only distributed transactions
// For writes, one has to revert to using locks.

namespace dstm {

typedef uint64_t Version;

class ObjectPublicHeader {
public:
	ObjectPublicHeader()
		: version_(0)
	{ }

protected:
	Version version_;
};


class ObjectPrivateHeader {
public:
	ObjectPrivateHeader()
		: version_(0)
	{ }

protected:
	Transaction* tx;
};


template<class Derived>
class Object: public ObjectPublicHeader {

};

// this is the private wrapper 
template<class Derived, class WrappedObject>
class ObjectWrapper: public Object<Derived> {
public:
	Derived* xOpenRO(Transaction* tx);

private:
	WrappedObject	
};


class Transaction {
public:
	int Begin();
	int	End();

private:
	typedef google::dense_hash_map<T*, Entry> Set;
	
	Set read_set_;
};

/*
// Read consistent set. implemented similarly to Harris, OOPSLA 2003
// 

// T: object type
template<typename T>
class OptReadSet {
	struct Entry {
		Entry(TimeStamp ts = 0)
			: old_ts_(ts)
		{ }

		TimeStamp old_ts_;
	};

	typedef google::dense_hash_map<T*, Entry> Set;

public:
	OptReadSet();
	void Reset();
	int Read(T* obj);
	bool Validate();
	
private:
	Set set_; 
};


template<typename T>
OptReadSet<T>::OptReadSet()
{
	set_.set_empty_key(0);
}


template<typename T>
void
OptReadSet<T>::Reset()
{
	set_.clear();
}


template<typename T>
int 
OptReadSet<T>::Read(T* obj) 
{
	typename Set::iterator it;

	if ((it = set_.find(obj)) != set_.end()) {
		return 0; // do nothing
	}

	set_[obj] = Entry(obj->ts());
	return 0;
}


template<typename T>
bool
OptReadSet<T>::Validate()
{
	typename Set::iterator it;
	T*                     obj;

	for (it = set_.begin(); it != set_.end(); it++) {
		obj = it->first;
		if (obj->ts() > it->second.old_ts_) {
			return false;
		}
	}
	return true;
}

*/

} // namespace dstm

#endif // _TRANSACTION_STAMNOS_H_BMA567