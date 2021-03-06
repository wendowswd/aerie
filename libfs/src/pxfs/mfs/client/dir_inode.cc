#define  __CACHE_GUARD__
#include "pxfs/mfs/client/dir_inode.h"
#include "pxfs/mfs/client/inode_factory.h"

#include <stdint.h>
#include "common/util.h"
#include "common/prof.h"
#include "pxfs/client/backend.h"
#include "pxfs/client/session.h"
#include "pxfs/client/const.h"
#include "pxfs/common/publisher.h"
#include <stdio.h>
//#define PROFILER_SAMPLE __PROFILER_SAMPLE

namespace mfs {
namespace client {

// FIXME: Should Link/Unlink increment/decrement the link count as well? currently the caller 
// must do a separate call, which breaks encapsulation. 

int
DirInode::return_dentry(::client::Session* session, void * inode_list_head)
{

	struct dentry dentry_head;

        struct list_item *head = (struct list_item *)inode_list_head;
        struct list_item *tail = (struct list_item *)head->data;
        struct list_item *tmp;

        dentry_head.val = (uint64_t) &dentry_head;
        dentry_head.next_dentry = NULL;
        rw_ref()->proxy()->interface()->return_dentry((void *)&dentry_head);
        
	struct dentry *it;
        ::client::Inode *ip;
        it = dentry_head.next_dentry;
        
	while(it){
                osd::common::ObjectId *oid = (osd::common::ObjectId *)it->val;
                InodeFactory::LoadInode(session, *oid, &ip);

                strcpy(ip->self_name, self_name);
                strcat(ip->self_name, "/");
                strcat(ip->self_name, it->key);
                ip->parent = this;
                tmp = new struct list_item;
                tmp->data = (void *)ip;
                tmp->next = NULL;

                tail->next = tmp;
                tail = tail->next;
                it = it->next_dentry;

        }
        ((struct list_item *)inode_list_head)->data = (void *)tail;

}

int 
DirInode::Lookup(::client::Session* session, const char* name, int flags, ::client::Inode** ipp) 
{
	PROFILER_PREAMBLE
	int                   ret;
	::client::Inode*      ip;
	osd::common::ObjectId oid;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_inode),
	        "Inode [%p, %lx]: lookup %s\n", this, ino(), name);

	assert(ref_ != NULL);

	// special case: requesting parent (name=..) and parent_ points to a pseudo-inode
	if (parent_ && str_is_dot(name) == 2) {
		ip = parent_;
		goto done;
	}
	PROFILER_SAMPLE
//	printf("\nInside DirInode::Lookup.");
	ip = NULL;
	if ((ret = rw_ref()->proxy()->interface()->Find(session, name, &oid, ip)) != E_SUCCESS) {
	// insight : This look up has to end in the persistent structure.
		return ret;
	}

//	if(ip) {

//	goto done;
//	}
      // insight : DO ME : Distinguish between a successful lookup in the ShadowCache and Collection
      // insight : DO ME : Do not proceed to LoadInode if we have a successful lookup in the ShadowCache
      //
	PROFILER_SAMPLE
	if ((ret = InodeFactory::LoadInode(session, oid, &ip)) != E_SUCCESS) {
		return ret;
	}
	PROFILER_SAMPLE
//	Link(session, name, ip, false);

done:
	ip->Get(); // insgiht : You should not be doing it here .
	*ipp = ip;
	return E_SUCCESS;
}


int 
DirInode::xLookup(::client::Session* session, const char* name, int flags, ::client::Inode** ipp) 
{
	int                   ret;
	::client::Inode*      ip;
	osd::common::ObjectId oid;
	
	DBG_LOG(DBG_INFO, DBG_MODULE(client_inode),
	        "Inode [%p, ino=%lx]: xLookup %s \n", this, ino(), name);

	assert(ref_ != NULL);
	// special case: requesting parent (name=..) and parent_ points to a pseudo-inode
	if (parent_ && str_is_dot(name) == 2) {
		ip = parent_;
		goto done;
	}

	if ((ret = rw_ref()->proxy()->xinterface()->Find(session, name, &oid)) != E_SUCCESS) {
		return ret;
	}
	if ((ret = InodeFactory::LoadInode(session, oid, &ip)) != E_SUCCESS) {
		return ret;
	}
done:
	ip->Get();
	*ipp = ip;
	return E_SUCCESS;
}



int 
DirInode::Link(::client::Session* session, const char* name, ::client::Inode* ip, 
               bool overwrite)
{
	int ret; 

	DBG_LOG(DBG_INFO, DBG_MODULE(client_inode),
	        "Inode [%p, %lx]: link %s -> (%p, ino=%lx)\n", this, ino(), name, ip, ip->ino());

	ip->nlink();

	// special case: if inode oid is zero then we link to a pseudo-inode. 
	// keep this link in the in-core state parent_
	if (ip->oid() == osd::common::ObjectId(0)) {
		pthread_mutex_lock(&mutex_);
		parent_ = ip;
		pthread_mutex_unlock(&mutex_);
		return E_SUCCESS;
	}
//	printf("\n @ Inserting %s into hash table.");
	if ((ret = rw_ref()->proxy()->interface()->Insert(session, name, ip->oid(), ip)) != E_SUCCESS) {
		return ret;
	}
	return E_SUCCESS;
}


int 
DirInode::Unlink(::client::Session* session, const char* name)  
{
	int ret; 

	DBG_LOG(DBG_INFO, DBG_MODULE(client_inode),
	        "Inode [%p, %lx]: unlink %s\n", this, ino(), name);

	if ((ret = rw_ref()->proxy()->interface()->Erase(session, name)) != E_SUCCESS) {
		return ret;
	}
	return E_SUCCESS;
}


/*
//  List all directory entries of a directory
//     This requires coordinating with the immutable directory inode. Simply
//     taking the union of the two inodes is not correct as some entries 
//     that appear in the persistent inode may have been removed and appear
//     as negative entries in the volatile cache. One way is for each entry 
//     in the persistent inode to check whether there is a corresponding negative
//     entry in the volatile cache. But this sounds like an overkill. 
//     Perhaps we need a combination of a bloom filter of just the negative entries 
//     and a counter of the negative entries. As deletes are rare, the bloom filter
//     should quickly provide an answer.

int 
DirInode::Readdir()
{

}
*/


int DirInode::nlink()
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_inode),
	        "In inode %lx, nlink = %d\n", ino(), rw_ref()->proxy()->interface()->nlink());
	
	return rw_ref()->proxy()->interface()->nlink();
}


int DirInode::set_nlink(int nlink)
{
	DBG_LOG(DBG_INFO, DBG_MODULE(client_inode),
	        "In inode %lx, set nlink = %d\n", ino(), nlink);
	
	return rw_ref()->proxy()->interface()->set_nlink(nlink);
}


int
DirInode::Lock(::client::Session* session, lock_protocol::Mode mode)
{
	s_log("[%ld] DirInode::%s (1)", s_tid, __func__);
	#ifdef FAST_SPIDER_LOCK_PATH
		pthread_mutex_lock(&spider_lock);
	#else
	osd::containers::client::NameContainer::Proxy* cc_proxy;

	if (ref_) {
		cc_proxy = rw_ref()->proxy();	
		return cc_proxy->Lock(session, mode);
	}
	#endif
	return E_SUCCESS;
}


int
DirInode::Lock(::client::Session* session, Inode* parent_inode, lock_protocol::Mode mode)
{
	#ifdef FAST_SPIDER_LOCK_PATH
		pthread_mutex_lock(&spider_lock);
	#else
	osd::containers::client::NameContainer::Proxy* cc_proxy;
	osd::containers::client::NameContainer::Proxy* cc_proxy_parent;
	s_log("[%ld] DirInode::%s (2)", s_tid, __func__);

	if (ref_) {
		cc_proxy = rw_ref()->proxy();	
		if (parent_inode->ref_) {
			// the parent_inode must always be of DirInode type, 
			// otherwise we can't do hierarchical locking right?
			DirInode* parent_dir_inode = dynamic_cast<DirInode*>(parent_inode);
			cc_proxy_parent = parent_dir_inode->rw_ref()->proxy();	
			return cc_proxy->Lock(session, cc_proxy_parent, mode);
		} else {
			return cc_proxy->Lock(session, mode);
		}
	}
	#endif
	return E_SUCCESS;
}


int
DirInode::Unlock(::client::Session* session)
{
	#ifdef FAST_SPIDER_LOCK_PATH
		pthread_mutex_unlock(&spider_lock);
	#else
	osd::containers::client::NameContainer::Proxy* cc_proxy;
	s_log("[%ld] DirInode::%s ", s_tid, __func__);

	if (ref_) {
		cc_proxy = rw_ref()->proxy();	
		return cc_proxy->Unlock(session);
	}
	#endif
	return E_SUCCESS;
}
 

int 
DirInode::xOpenRO(::client::Session* session)
{
	rw_ref()->proxy()->xOpenRO();
	return E_SUCCESS;
}


int 
DirInode::Sync(::client::Session* session)
{
	return rw_ref()->proxy()->interface()->vUpdate(session);
}


int 
DirInode::ioctl(::client::Session* session, int request, void* info)
{
	int ret = E_SUCCESS;
	switch (request)
	{
		case kIsEmpty: {
			// empty if only entries is self (.) and parent (..)
			bool isempty = (rw_ref()->proxy()->interface()->Size(session) > 2) ? false: true;
			*((bool *) info) = isempty;
		} break;
	}
	return ret;
}


} // namespace client
} // namespace mfs
