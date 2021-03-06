#include "tool/testfw/unittest.h"
#include "common/errno.h"
#include "scm/pool/pool.h"

#include <stdio.h>

SUITE(SCM)
{
	TEST(TestPool)
	{
		StoragePool* pool1;
		void*        ptr;

		CHECK(StoragePool::Create("/tmp/stamnos_pool", 1024*1024*1024, 0) == E_SUCCESS);
		CHECK(StoragePool::Open("/tmp/stamnos_pool", &pool1) == E_SUCCESS);

		CHECK(pool1->AllocateExtent(4096*2, &ptr) == E_SUCCESS);
		CHECK(pool1->AllocateExtent(4096*5, &ptr) == E_SUCCESS);
		
		CHECK(StoragePool::Close(pool1) == E_SUCCESS);

		CHECK(StoragePool::Open("/tmp/stamnos_pool", &pool1) == E_SUCCESS);
		CHECK(pool1->AllocateExtent(4096*3, &ptr) == E_SUCCESS);
	}
}
