#ifndef __STAMNOS_BCS_SERVER_SHARED_BUFFER_H
#define __STAMNOS_BCS_SERVER_SHARED_BUFFER_H

#include <stddef.h>
#include <string>
#include "bcs/main/common/shbuf.h"

namespace server {

class SharedBuffer {
public:
	int Init(const char* suffix);
	SharedBufferDescriptor Descriptor() {
		return SharedBufferDescriptor(id_, path_, size_);
	}

	virtual int Consume();

private:
	int         id_; // an identifier local to the client assigned the buffer
	void*       base_;
	size_t      size_;
	std::string path_;
};


} // namespace server

#endif // __STAMNOS_BCS_SERVER_SHARED_BUFFER_H
