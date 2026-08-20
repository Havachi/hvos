#ifndef _HVOS_KERNEL_FS_EXT2_H
#define _HVOS_KERNEL_FS_EXT2_H

#include "data_structure/bitmap.h"
#include "kernel/fs/block_dev.h"
#include <stdint.h>
#include <sys/cdefs.h>
#include "data_structure/uuid.h"
#include "mem/mem.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define EXT2_SIGNATURE 0xEF53
#define BLOCK_PER_GROUP 8192
#define INODE_PER_GROUP 2048
#define LOG_BLOCK_SIZE 0
#define EXT2_NAME_LEN 255
#define EXT2_RESERVED_INODES 10
#define MAX_PATH_DEPTH 255

#define C_BLKSZ(s_log_block_size) (1024 << s_log_block_size)

#define EXT2_INO_TO_GROUP(ino, ino_per_grp) \
	(((ino) - 1) / (ino_per_grp))

#define EXT2_INO_TO_BIT_IDX(ino, ino_per_grp) \
	(((ino) - 1) % (ino_per_grp))

//Dirs helper macros
#define EXT2_DIR_REC_LEN(name_len)	(((8 + (name_len) + 3) >> 2) << 2)
#define EXT2_FT_DIR					2

//Reserved Inodes
#define EXT2_BAD_INO 1
#define EXT2_ROOT_INO 2
#define EXT2_ACL_IDX_INO 3
#define EXT2_ACL_DATA_INO 4
#define EXT2_BOOT_LOADER_INO 5
#define EXT2_UNDEL_DIR_INO 6

#define EXT2_DIRECT_BLOCKS 12
#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK 12
#define EXT2_DIND_BLOCK 13
#define EXT2_TIND_BLOCK 14
#define S_IFMT 0xF000

//Return 1 if the version of ext2 is high enough for the sb to have extended fields
#define ISSBEXT(sb) (sb->base.v_major >= 1) ? 1 : 0;

//Ext2 superblock fields
typedef struct {
	//Total number of inodes in file system
	uint32_t s_inodes_count;
	//Total number of blocks in file system
	uint32_t s_blocks_count;
	//Number of blocks reserved for superuser 
	uint32_t s_r_blocks_count;
	//Total number of unallocated blocks
	uint32_t s_free_blocks_count;
	//Total number of unallocated inode
	uint32_t s_free_inode_count;
	//Block number of the block containing the superblock
	uint32_t s_first_data_block;
	//log2(block size) - 10
	uint32_t s_log_block_size;
	//log2(fragment size) - 10
	uint32_t s_log_frag_size;
	//Number of blocks in each block group
	uint32_t s_blocks_per_group;
	//Number of fragment in each block group
	uint32_t s_frags_per_group;
	//Number of inodes in each block group
	uint32_t s_ine_per_group;
	//Last mount time
	uint32_t s_mtime;
	//Last write time
	uint32_t s_wtime;
	//Number of time the volume has been mounted since its last consistency check
	uint16_t s_mnt_count;
	//Number of mounts allowed before a consistency check must be done
	uint16_t s_max_mnt_count;
	//Ext2 signature (0xef53)
	uint16_t s_magic;
	//File system state
	uint16_t s_state;
	//What to do when an error is detected
	uint16_t s_errors;
	//Minor portion of version
	uint16_t s_minor_rev_level;
	//time of last consistency check
	uint32_t s_lastcheck;
	//Interval between forced consistency checks;
	uint32_t s_checkinterval;
	//Operating system ID
	uint32_t s_creator_os;
	//Major portion of version
	uint32_t s_rev_level;
	//User ID that can use reserved blocks
	uint16_t s_def_resuid;
	//Group ID that can use reserved blocks
	uint16_t s_def_resgid;
} __packed ext2_base_sb_t;

//Ext2 extended superblock fields
typedef struct {
	/*-- EXT2_DYNAMIC_REV Specific--*/
	//First non-reserved inode in filesystem
	uint32_t s_first_ino;
	//Size of each inode structure in bytes
	uint16_t s_inode_size;
	//Block group that this superblock is part of (if backup copy)
	uint16_t s_block_group_nr;
	//Optional features present
	uint32_t s_feature_compat;
	//Required features present
	uint32_t s_freature_incompat;
	//Features that if no supported, the volume must be mounted read-only
	uint32_t s_feature_ro_compat;
	//File system ID
	uint8_t uuid[16];
	//Volume name (C-String, null terminated)
	uint8_t s_volume_name[16];
	//Path volume was last mounted to (C-String, null terminated)
	uint8_t s_last_mounted[64];
	//Compression algorithm used
	uint32_t s_algo_bitmap;

	/*-- Performence Hints --*/
	//Number of block to preallocate for files
	uint8_t s_prealloc_blocks;
	//Number of block to preallocate for directories
	uint8_t s_prealloc_dir_blocks;
	//Unused (alignment)
	uint16_t _unused1;
	
	/*-- Journaling support --*/
	//Journal ID (same style as the file system ID above)
	uint8_t s_journal_uuid[16];
	//Journal inode
	uint32_t s_journal_inum;
	//Journal device
	uint32_t s_jorunal_dev;
	//Head of orphan inode list
	uint32_t s_last_orphan;

	/*-- Directory Indexing support --*/
	uint32_t	s_hash_seed[4];
	uint8_t		s_def_hash_version;
	uint8_t		_unused2[3];
	/*-- Other options --*/
	uint32_t	s_default_mount_option;
	uint32_t	s_first_meta_log;
}__packed ext2_ext_sb_t;


typedef struct {
	ext2_base_sb_t base;
	ext2_ext_sb_t ext;
} __packed __aligned(1024) ext2_sb_t;


//Ext2 block group descriptor
typedef struct {

	//Block address of block usage bitmap
	uint32_t bg_block_bitmap;
	//Block address of inode usage bitmap
	uint32_t bg_inode_bitmap;
	//Starting block address of inode table
	uint32_t bg_inode_table;
	//Number of unallocated blocks in group
	uint16_t bg_free_blocks_count;
	//Number of unallocated inodes in group
	uint16_t bg_free_inodes_count;
	//Number of directories in group
	uint16_t bg_used_dirs_count;
	//padding
	uint16_t bg_pad;
	uint8_t bg_reserved[12];
} __packed ext2_bgd_t;

typedef struct {
	block_device_t 	*dev;
	ext2_sb_t		*sb;
	ext2_bgd_t		*bgdt;
	uint32_t		block_size;
	uint32_t		block_count;
	uint32_t		inodes_per_block;
	uint32_t		pointer_per_block;
	uint32_t		bgdt_blocks;
	uint32_t		group_count;
	bool			sb_dirty;
	bool			bgdt_dirty;
} ext2_fs_t;

//Ext2 inode
typedef struct {
	//Type and permissions
	uint16_t i_mode;
	//User ID
	uint16_t i_uid;
	//Lower 32 bits of size in bytes 
	uint32_t i_size;
	//Last access time
	uint32_t i_atime;
	//Creation time
	uint32_t i_ctime;
	uint32_t i_mtime;
	//Deletion time
	uint32_t i_dtime;
	//Group ID;
	uint16_t i_gid;
	//Count of hard links (directory entries) to this inode. When this reaches 0, the data blocks are marked as unallocated
	uint16_t i_links_count;
	//Count of disk sectors in use by this inode, not counting the actual inode strucutre nor directory entries linking to the inode.
	uint32_t i_blocks;
	//Flags
	uint32_t i_flags;
	//Operating system specific value #1, can safely be 0
	uint32_t i_osd1;
	uint32_t i_block[15];
	uint32_t i_generation;
	//File ACL
	uint32_t i_file_acl;
	uint32_t i_dir_acl;
	//Block address of fragment
	uint32_t i_faddr;
	//Operating System specific value #2
	char i_osd2[12];
} __packed ext2_ine_t;

typedef struct {
	//Inode
	uint32_t		inode;
	//Total size of this entry
	uint16_t		rec_len;
	//Name length least-significant 8 bits
	uint8_t			name_len;
	uint8_t			file_type;
	//Name characters
	char			name[];
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


/*bgdt.c*/
int ext2_bgdt_read_entry(ext2_fs_t *fs, uint32_t bg_id, ext2_bgd_t *out);
int ext2_bgdt_read_entry_raw(block_device_t *dev, uint32_t bg_id, ext2_bgd_t *out);
int ext2_bgdt_write_entry(ext2_fs_t *fs, uint32_t bg_id, const ext2_bgd_t *entry);
int mkfs_ext2_bgdt_init(block_device_t *dev, ext2_sb_t *sb, ext2_bgd_t *grp0_bgd);
int blkgrp_has_super(uint32_t group);
int ext2_bgdt_sync(ext2_fs_t *fs);
/* blk_io.c */
int ext2_read_block(ext2_fs_t *fs, uint32_t blk_id, uint8_t *buf);
int ext2_read_blocks(block_device_t *dev, uint32_t blk_start, uint32_t blk_end, uint8_t *buf);
int ext2_read_block_ptr(ext2_fs_t *fs, uint32_t pblk, uint32_t *buf);
int ext2_write_block(ext2_fs_t *fs, uint32_t blk_id, const uint8_t *buf);
int ext2_write_blocks(block_device_t *dev, uint32_t blk_start, uint32_t blk_end, const uint8_t *buf);
int ext2_write_block_ptr(ext2_fs_t *fs, uint32_t pblk, const uint32_t *buf);
int ext2_alloc_block(ext2_fs_t *fs, uint32_t group_hint, uint32_t *out_blk_id);
int ext2_free_block(ext2_fs_t *fs, uint32_t pblk);
/*dir.c*/
int ext2_dir_find_entry(ext2_fs_t *fs, const ext2_ine_t *dir_inode, const char *name, ino_t *out_ino);
int ext2_dir_add_entry(ext2_fs_t *fs, ino_t dir_ino, const char *name, ino_t ino_id, uint8_t file_type);
int ext2_dir_remove_entry(ext2_fs_t *fs, ino_t dir_ino, const char *name);
int ext2_dir_readdir(ext2_fs_t *fs, const ext2_ine_t *dir_inode, uint32_t *offset, ext2_dir_t *out_entry);

/*ext2.c*/
int mkfs_ext2(block_device_t *dev);
bitmap_t *ext2_read_bitmap(block_device_t *dev, uint32_t loc, uint32_t total_bits);
uint32_t ext2_write_bitmap(block_device_t *dev, uint32_t loc, uint32_t total_bits, bitmap_t *map);
int ext2_mount(block_device_t *dev, ext2_fs_t **out_fs);
int ext2_unmount(ext2_fs_t *fs);
int ext2_sync(ext2_fs_t *fs);
int ext2_lookup_path(ext2_fs_t *fs, const char *path, ino_t cwd_ino, ino_t *out_ino);
int ext2_bmap(ext2_fs_t *fs, ext2_ine_t *inode, uint32_t lblk, int create_if_missing, uint32_t *out_pblk);

/*ino.c*/
int ext2_inode_read(ext2_fs_t *fs, ino_t ino_id, ext2_ine_t *out);
int ext2_inode_write(ext2_fs_t *fs, ino_t ino_id, const ext2_ine_t *inode);
int ext2_inode_alloc(ext2_fs_t *fs, uint32_t group_hint, uint16_t mode, ino_t *out_ino_id);
int ext2_inode_free(ext2_fs_t *fs, ino_t ino_id);
int ext2_free_inode_blocks(ext2_fs_t *fs, ext2_ine_t *inode);
/*sb.c*/
int ext2_sb_read(ext2_fs_t *fs, ext2_sb_t *out_sb);
int ext2_sb_read_raw(block_device_t *dev, ext2_sb_t *out_sb);
int ext2_sb_write(ext2_fs_t *fs, const ext2_sb_t *sb);
int ext2_sb_write_raw(block_device_t *dev, const ext2_sb_t *sb);
int ext2_sb_sync(ext2_fs_t *fs);

int ext2_update_sb(ext2_fs_t *fs, const ext2_sb_t *sb);
int ext2_update_sb_raw(block_device_t *dev, const ext2_sb_t *sb);
/*VFS API*/


//Return codes
#define OPERATION_SUCCESS 0
#define OPERATION_FAILED 1
#define IO_ERROR 4
#define DEVICE_READ_ERROR 5
#define DEVICE_WRITE_ERROR 6
#define DEVICE_NEED_REFORMAT 7
#define DEVICE_UNAVAILABLE 8

#endif