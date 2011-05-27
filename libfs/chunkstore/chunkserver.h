#ifndef _CHUNKSERVER_H_AGT127
#define _CHUNKSERVER_H_AGT127

#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>
#include <map>
#include "lockserv/lockserv.h"
#include "common/pheap.h"
#include "common/vistaheap.h"
#include "chunkdsc.h"


class ChunkServer {
public:
	void Init();
	int CreateChunk(int principal_id, size_t size, ChunkDescriptor** chunkdscp);
	int DeleteChunk(int principal_id, ChunkDescriptor* chunkdsc);
	int AccessChunk(int principal_id, ChunkDescriptor* chunkdsc);
    int AccessAddr(int principal_id, void* addr);

	ChunkServer();
private:
	id_t                                     _principal_id;
	PHeap*                                   _pheap; 
	PHeap*                                   _pagepheap; 
	pthread_mutex_t                          _mutex;
	std::map<unsigned long long, 
	         ChunkDescriptor*> _addr2chunkdsc_map; 
};

#endif