#ifndef __STAMNOS_PXFS_MFS_SERVER_PUBLISHER_H
#define __STAMNOS_PXFS_MFS_SERVER_PUBLISHER_H

#include "ssa/main/server/ssa-opaque.h"
#include "pxfs/common/publisher.h"

namespace server {

class WriteVerifier;

class Publisher {
public:
	static int Init();
	static int Register(::ssa::server::StorageSystem* stsystem);
	static int Write(::ssa::server::SsaSession* session, char* lgc_op_hdr, ::ssa::Publisher::Messages::BaseMessage* next);

private:
	static WriteVerifier* write_verifier_;
};


} // namespace server

#endif  // __STAMNOS_PXFS_MFS_SERVER_PUBLISHER_H
