#ifndef __OSDCLUSTER_H
#define __OSDCLUSTER_H

/*
 * describe properties of the OSD cluster.
 *   disks, disk groups, total # osds,
 *   whether we're in limbo/recovery state, etc.
 *
 */

#define NUM_REPLICA_GROUPS   1024   // ~1000 i think?
#define NUM_RUSH_REPLICAS      10   // this should be big enough to cope w/ failing disks.

#define FILE_OBJECT_SIZE   (1<<20)  // 1 MB object size

#define OID_BLOCK_BITS     30       // 1mb * 1^9 = 1 petabyte files
#define OID_INO_BITS       (64-30)  // 2^34 =~ 16 billion files

#define MAX_FILE_SIZE      (FILE_OBJECT_SIZE << OID_BLOCK_BITS)  // 1 PB


/** OSDGroup
 * a group of identical disks added to the OSD cluster
 */
class OSDGroup {
  int         size;     // num disks in this group           (aka num_disks_in_cluster[])
  int         weight;   // weight (for data migration etc.)  (aka weight_cluster[])
  vector<int> osds;     // the list of actual osd's
};


/** OSDCluster
 */
class OSDCluster {
  __uint64_t version;           // what version of the osd cluster descriptor is this

  // RUSH disk groups
  vector<OSDGroup> osd_groups;  // RUSH disk groups
  set<int>         failed_osds; // list of failed disks


 public:
  OSDCluster() : version(0) { }

  // cluster state
  bool is_failed(int osd) { return failed_osds.count(osd) ? true:false; }
  
  int num_osds() {
	int n = 0;
	for (vector<OSDGroup>::iterator it = osd_groups.begin();
		 it != osd_groups.end();
		 it++) 
	  n += it->size;
	return n;
  }

  // mapping facilities

  /* map (ino) into a replica group */
  repgroup_t file_to_repgroup(inodeno_t ino) {
	// something simple.  ~100 rg's.
	return ino % NUM_REPLICA_GROUPS;
  }

  /* map a repgroup to a list of osds.  
	 this is where we use RUSH! */
  int repgroup_to_osds(repgroup_t rg,
					   list<int>& osds,   // list of osd addr's
					   int num_rep) {     // num replicas
	// do something simple for now
	osds.clear();
	for (int i=0; i<num_rep; i++) {
	  // mask out failed disks
	  int osd = MSG_ADDR_OSD( (rg+i) % num_osds() );
	  if (is_failed(osd)) continue;
	  osds.push_back( osd );
	}

	return 0;
  }

  /* map (ino, block) to an object name
	 (to be used on any osd in the proper replica group) */
  object_t file_to_object(inodeno_t ino,
						  size_t    blockno) {  
	assert(ino < (1<<OID_INO_BITS));       // legal ino can't be too big
	assert(blockno < (1<<OID_BLOCK_BITS));
	return (ino << OID_INO_BITS) + blockno;
  }

  
  // serialize, unserialize
  void _rope(crope& r);
  void _unrope(crope& r, int& off);
};


#endif
