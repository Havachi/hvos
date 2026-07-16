#ifndef _HVOS_KERNEL_FS_EXT2_H
#define _HVOS_KERNEL_FS_EXT2_H

#include <stdint.h>
#include <sys/cdefs.h>

#define EXT2_SIGNATURE 0xEF53

//Return 1 if the version of ext2 is high enough for the sb to have extended fields
#define ISSBEXT(sb) (sb->base.v_major >= 1) ? 1 : 0;

//Ext2 superblock fields
typedef struct {
	//Total number of inodes in file system
	uint32_t total_ine;
	//Total number of blocks in file system
	uint32_t total_blk;
	//Number of blocks reserved for superuser 
	uint32_t nb_rsv_blk;
	//Total number of unallocated blocks
	uint32_t total_un_blk;
	//Total number of unallocated inode
	uint32_t total_un_ine;
	//Block number of the block containing the superblock
	uint32_t blk_sb;
	//log2(block size) - 10
	uint32_t log2_blk_sz;
	//log2(fragment size) - 10
	uint32_t log2_frg_sz;
	//Number of blocks in each block group
	uint32_t nb_blk_grp;
	//Number of fragment in each block group
	uint32_t nb_frg_grp;
	//Number of inodes in each block group
	uint32_t nb_ine_grp;
	//Last mount time
	uint32_t mnt_time;
	//Last write time
	uint32_t wr_time;
	//Number of time the volume has been mounted since its last consistency check
	uint16_t nb_mnt_chk;
	//Number of mounts allowed before a consistency check must be done
	uint16_t nb_mnt_a_chk;
	//Ext2 signature (0xef53)
	uint16_t magic;
	//File system state
	uint16_t fs_state;
	//What to do when an error is detected
	uint16_t err_hndl;
	//Minor portion of version
	uint16_t v_min;
	//time of last consistency check
	uint32_t chk_time;
	//Interval between forced consistency checks;
	uint32_t itr_chk;
	//Operating system ID
	uint32_t os_id;
	//Major portion of version
	uint32_t v_major;
	//User ID that can use reserved blocks
	uint16_t uid;
	//Group ID that can use reserved blocks
	uint16_t gid;
} __packed ext2_base_sb_t;

//Ext2 extended superblock fields
typedef struct {
	//First non-reserved inode in filesystem
	uint32_t fnri;
	//Size of each inode structure in bytes
	uint16_t sz_ine;
	//Block group that this superblock is part of (if backup copy)
	uint16_t sb_blk_grp;
	//Optional features present
	uint32_t opt_feat;
	//Required features present
	uint32_t req_feat;
	//Features that if no supported, the volume must be mounted read-only
	uint32_t ro_feat;
	//File system ID
	char fsid[16];
	//Volume name (C-String, null terminated)
	char vol_name[16];
	//Path volume was last mounted to (C-String, null terminated)
	char vol_path[64];
	//Compression algorithm used
	uint32_t compr_alg;
	//Number of block to preallocate for files
	char nb_blk_pre_alloc_file;
	//Number of block to preallocate for directories
	char nb_blk_pre_alloc_dir;
	//Unused
	uint16_t _unused1;
	//Journal ID (same style as the file system ID above)
	char journal_id[16];
	//Journal inode
	uint32_t jrnl_ine;
	//Journal device
	uint32_t jrnl_dev;
	//Head of orphan inode list
	uint32_t head_orph_ine;
}__packed ext2_ext_sb_t;


typedef struct {
	ext2_base_sb_t base;
	ext2_ext_sb_t ext;
	uint32_t padding;
} __packed __aligned(1024) ext2_sb_t;


//Ext2 block group descriptor
typedef struct {

	//Block address of block usage bitmap
	uint32_t addr_blk_usage_bm;
	//Block address of inode usage bitmap
	uint32_t addr_ine_usage_bm;
	//Starting block address of inode table
	uint32_t addr_ine_tbl;
	//Number of unallocated blocks in group
	uint16_t nb_unalloc_blk;
	//Number of unallocated inodes in group
	uint16_t nb_unalloc_ine;
	//Number of directories in group
	uint16_t nb_dir;

} __packed ext2_bgd_t;

//Ext2 inode
typedef struct {
	//Type and permissions
	uint16_t type_perm;
	//User ID
	uint16_t uid;
	//Lower 32 bits of size in bytes 
	uint32_t sz_lo;
	//Last access time
	uint32_t a_time;
	//Creation time
	uint32_t c_time;
	//Deletion time
	uint32_t d_time;
	//Group ID;
	uint16_t gid;
	//Count of hard links (directory entries) to this inode. When this reaches 0, the data blocks are marked as unallocated
	uint16_t hlnk_cnt;
	//Count of disk sectors in use by this inode, not counting the actual inode strucutre nor directory entries linking to the inode.
	uint32_t sect_cnt;
	//Flags
	uint32_t flags;
	//Operating system specific value #1, can safely be 0
	uint32_t os_v_1;
	//Direct block pointer 0-11
	uint32_t dbp[12];
	//Singly Indirect Block Pointer (Points to a block that is a list of block pointers to data)
	uint32_t sibp;
	//Doubly Indirect Block Pointer (Points to a block that is a list of block pointers to Singly Indirect Blocks)
	uint32_t dibp;
	//Triply Indirect Block Pointer (Points to a block that is a list of block pointers to Doubly Indirect Blocks)
	uint32_t tibp;
	//Generation number (Primarily used for NFS)
	uint32_t gen_num;
	//File ACL
	uint32_t f_acl;
	union {
		//higher 32 bits of size in bytes (if feature bit set)
		uint32_t sz_hi;
		//Directory ACL if it's a directory
		uint32_t d_acl
	};
	//Block address of fragment
	uint32_t blk_addr_frg;
	//Operating System specific value #2
	char os_v_2[12];
} __packed ext2_ine_t;

typedef struct {
	//Inode
	uint32_t	ine;
	//Total size of this entry
	uint16_t	size;
	//Name length least-significant 8 bits
	uint8_t		name_sz_lo;
	union {
		//Only if feature bit for "directory entries have file type byte" is set
		uint8_t		type;
		//Most significant 8bits of the Name Length
		uint8_t		name_sz_hi;
	};
	//Name characters
	char *		name;
} __packed ext2_dir_t;

enum {
	//File system is clean
	EXT2_STATE_CLEAN=1,
	//File system has errors
	EXT2_STATE_ERROR=2,
};

enum {
	//Ignore the error (continue on)
	EXT2_ERR_IGNORE = 1,
	//Remount file system as read-only
	EXT2_ERR_REMOUNT = 2,
	//Kernel panic
	EXT2_ERR_PANIC = 3,
};

enum {
	OSID_LINUX = 0,
	OSID_GNUHURD = 1,
	OSID_MASIX = 2,
	OSID_FREEBSD = 3,
	OSID_OTHER = 4,
};

enum {
	//Preallocate some number of blocks to a directory when creating a new one
	OPT_FEAT_PREALLOC_BLOCKS = 0x0001,
	//AFS server inode exist
	OPT_FEAT_AFS_SRV_INE = 0x0002,
	//File system has journal
	OPT_FEAT_FS_JRNL = 0x0004,
	//Inodes have extended attributes
	OPT_FEAT_INE_EXT = 0x0008,
	//File system can resize itself for larger partitions
	OPT_FEAT_RESIZE = 0x0010,
	//Directories use hash index
	OPT_FEAT_HASHIDX = 0x0020,
};

enum {
	//Compression is used
	REQ_FEAT_COMPR = 0x0001,
	//Directory entries contain a type field
	REQ_FEAT_DIR_HAS_TYPE = 0x0002,
	//File system needs to replay its journal
	REQ_FEAT_FS_JRNL_REPLAY = 0x0004,
	//File system uses a journal device
	REQ_FEAT_FS_JRNL_DEV = 0x0008,
};

enum {
	//Sparse superblock and groupe descriptor tables
	RO_FEAT_SPARSE = 0x0001,
	//File system uses a 64-bit file size
	RO_FEAT_64BITS = 0x0002,
	//Directory contents are stored in the form of a Binary Tree
	RO_FEAT_BINTREE = 0x0004,
};

enum {
	INE_TYPE_FIFO = 0x1000,
	INE_TYPE_CHARDEV = 0x2000,
	INE_TYPE_DIR = 0x4000,
	INE_TYPE_BLKDEV = 0x6000,
	INE_TYPE_FILE = 0x8000,
	INE_TYPE_SYMLNK = 0xA000,
	INE_TYPE_SOCKET = 0xC000,
};

enum {
	DIR_TYPE_UKN = 0x0,
	DIR_TYPE_FILE = 0x1,
	DIR_TYPE_DIR = 0x2,
	DIR_TYPE_CHARDEV = 0x3,
	DIR_TYPE_BLKDEV = 0x4,
	DIR_TYPE_FIFO = 0x5,
	DIR_TYPE_SOCKET = 0x6,
	DIR_TYPE_SYMLNK = 0x7,
};

enum {
	//Other - Execute
	INE_PERM_OX = 0x001,
	//Other - Write
	INE_PERM_OW = 0x002,
	//Other - Read
	INE_PERM_OR = 0x004,


	//Group - Read
	INE_PERM_GX = 0x008,
	//Group - Write
	INE_PERM_GW = 0x010,
	//Group - Execute
	//Group - Execute
	INE_PERM_GR = 0x020,

	//User - Read
	INE_PERM_UX = 0x040,
	//User - Write
	INE_PERM_UW = 0x080,
	//User - Execute
	INE_PERM_UR = 0x100,

	//Sticky bit
	INE_PERM_SB = 0x200,
	//Set group ID
	INE_PERM_SG = 0x400,
	//Set user ID
	INE_PERM_SU = 0x800,
};

enum {
	//Secure deletion (not used)
	INE_FLAG_SDEL = 0x00000001,
	//Keep a copy of data when deleted (not used)
	INE_FLAG_KDEL = 0x00000002,
	//File compression (not used)
	INE_FLAG_COMP = 0x00000004,
	//Synchronous updates—new data is written immediately to disk
	INE_FLAG_SYNC = 0x00000008,
	//Immutable file (content cannot be changed)
	INE_FLAG_IMMU = 0x00000010,
	//Append only
	INE_FLAG_APPE = 0x00000020,
	//File is not included in 'dump' command
	INE_FLAG_NDUM = 0x00000040,
	//Last accessed time should not updated
	INE_FLAG_NTIM = 0x00000080,
	//Hash indexed directory
	INE_FLAG_HSHD = 0x00010000,
	//AFS directory
	INE_FLAG_AFSD = 0x00020000,
	//Journal file data
	INE_FLAG_JRFD = 0x00040000,
};


/*sb.c*/
ext2_sb_t *alloc_sb();
ext2_sb_t *phys_alloc_sb();
ext2_sb_t *new_sb();
ext2_sb_t *init_sb(ext2_sb_t *sb);
ext2_sb_t *read_sb();
void write_sb(ext2_sb_t *sb);
void init_ext2();
#endif