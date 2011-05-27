#include "chunkserver.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "common/util.h"
#include "common/vistaheap.h"
#include "chunkdsc.h"


const uint64_t kChunkStoreSize = 1024*1024*64;
const char*    kChunkStoreName = "chunkstore.vistaheap";

ChunkServer::ChunkServer():
	_principal_id(0)
{
	
}


void 
ChunkServer::Init()
{
	int        ret;
	PHeap*     pheap;

	pthread_mutex_init(&_mutex, NULL);
	
	_pheap = new PHeap();
	_pagepheap = new PHeap();
	_pheap->Open("chunkstore.pheap", 1024*1024, 0, 0, 0);
	_pagepheap->Open("chunkstore.pagepheap", kChunkStoreSize, 0, kPageSize, _pheap);

}


int
ChunkServer::CreateChunk(int principal_id, size_t size, ChunkDescriptor **chunkdscp)
{
	int                round_size; 
	ChunkDescriptor*   chunkdsc;
	void*              chunk;
	int                ret;
	std::pair<std::map<unsigned long long, ChunkDescriptor*>::iterator, bool> pairret;

	round_size = num_pages(size) * kPageSize;
	
	pthread_mutex_lock(&_mutex);	
	/* 
	 * FIXME: These two allocations have to be atomic. 
	 * We should follow the Ganger rules for allocating storage. 
	 */ 
	if ((ret = _pagepheap->Alloc(round_size, &chunk)) != 0) {
		pthread_mutex_unlock(&_mutex);	
		return -1;
	}
	assert((uintptr_t) chunk % kPageSize == 0);
	if ((ret = _pheap->Alloc(sizeof(*chunkdsc), (void**)&chunkdsc)) != 0) {
		_pagepheap->Free(chunk, round_size);
		pthread_mutex_unlock(&_mutex);	
		return -1;
	}	
	chunkdsc->_owner_id = _principal_id;
	chunkdsc->_chunk = chunk;
	chunkdsc->_size = round_size;

	/* Keeping the chunk intervals in a map is okay as they don't overlap */
	pairret = _addr2chunkdsc_map.insert(std::pair<unsigned long long, ChunkDescriptor*>((unsigned long long) chunk, chunkdsc));
	assert(pairret.second == true);
	*chunkdscp = chunkdsc;
	pthread_mutex_unlock(&_mutex);	

	printf("ChunkServer::CreateChunk: chunkdsc=%p (%llx, %llx)\n", chunkdsc, (unsigned long long) ((chunkdsc)->_chunk), (unsigned long long) ((chunkdsc)->_chunk) + size);
	return 0;
}


int 
ChunkServer::DeleteChunk(int principal_id, ChunkDescriptor* chunkdsc)
{
	int   ret;
	int   round_size;
	void* chunk;

	if (!chunkdsc) {
		return -1;
	}

	pthread_mutex_lock(&_mutex);	

	round_size = chunkdsc->_size;
	chunk = chunkdsc->_chunk;

	/* FIXME: access right whether you can free the chunk...
	 * Nevermind, this layer is gonna be implemented in the OS anyways
	 */
	_pheap->Free(chunkdsc, sizeof(*chunkdsc));
	_pagepheap->Free(chunk, round_size);
	ret = _addr2chunkdsc_map.erase((unsigned long long) chunk);
	assert(ret > 0);
	pthread_mutex_unlock(&_mutex);	
}


int 
ChunkServer::AccessChunk(int principal_id, ChunkDescriptor* chunkdsc)
{
/*
	std::map<unsigned long long, ChunkMetadata*>::iterator it;
	ChunkMetadata*                                         chunk_metadata;
	unsigned int                                            owner_id;

	it = chunk_metadata_map_.find(chunk_id);
	if (it == chunk_metadata_map_.end()) {
		return -1;
	}	
	chunk_metadata = it->second;
	owner_id = chunk_metadata->get_owner_id();

	return 0;
*/
}


int 
ChunkServer::AccessAddr(int principal_id, void* addr)
{
	unsigned long long                                       uaddr;
	std::map<unsigned long long, ChunkDescriptor*>::iterator it;
	std::map<unsigned long long, ChunkDescriptor*>::iterator itlow;
	ChunkDescriptor*                                         chunkdsc;

	uaddr = (unsigned long long) addr;

	itlow = _addr2chunkdsc_map.lower_bound(uaddr);
	for (it = --itlow; it != _addr2chunkdsc_map.end(); it++) 	{
		chunkdsc = it->second;
		if (uaddr >= (unsigned long long) chunkdsc->_chunk &&
		    uaddr < (unsigned long long) chunkdsc->_chunk + chunkdsc->_size)
		{
			return 0;
		}
	}
	
	return -1;
}