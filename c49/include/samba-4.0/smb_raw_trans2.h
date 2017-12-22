/* 
   Unix SMB/CIFS implementation.
   SMB transaction2 handling
   Copyright (C) Jeremy Allison 1994-2002.
   Copyright (C) Andrew Tridgell 1995-2003.
   Copyright (C) James Peach 2007

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _TRANS2_H_
#define _TRANS2_H_


/* trans2 Query FS info levels */
/*
w2k3 TRANS2ALIASES:
Checking for QFSINFO aliases
        Found level    1 (0x001) of size 18 (0x12)
        Found level    2 (0x002) of size 12 (0x0c)
        Found level  258 (0x102) of size 26 (0x1a)
        Found level  259 (0x103) of size 24 (0x18)
        Found level  260 (0x104) of size  8 (0x08)
        Found level  261 (0x105) of size 20 (0x14)
        Found level 1001 (0x3e9) of size 26 (0x1a)
        Found level 1003 (0x3eb) of size 24 (0x18)
        Found level 1004 (0x3ec) of size  8 (0x08)
        Found level 1005 (0x3ed) of size 20 (0x14)
        Found level 1006 (0x3ee) of size 48 (0x30)
        Found level 1007 (0x3ef) of size 32 (0x20)
        Found level 1008 (0x3f0) of size 64 (0x40)
Found 13 levels with success status
        Level 261 (0x105) and level 1005 (0x3ed) are possible aliases
        Level 260 (0x104) and level 1004 (0x3ec) are possible aliases
        Level 259 (0x103) and level 1003 (0x3eb) are possible aliases
        Level 258 (0x102) and level 1001 (0x3e9) are possible aliases
Found 4 aliased levels
*/
#define SMB_QFS_ALLOCATION		                   1
#define SMB_QFS_VOLUME			                   2
#define SMB_QFS_VOLUME_INFO                            0x102
#define SMB_QFS_SIZE_INFO                              0x103
#define SMB_QFS_DEVICE_INFO                            0x104
#define SMB_QFS_ATTRIBUTE_INFO                         0x105
#define SMB_QFS_UNIX_INFO                              0x200
#define SMB_QFS_POSIX_INFO                             0x201
#define SMB_QFS_POSIX_WHOAMI                           0x202
#define SMB_QFS_VOLUME_INFORMATION			1001
#define SMB_QFS_SIZE_INFORMATION			1003
#define SMB_QFS_DEVICE_INFORMATION			1004
#define SMB_QFS_ATTRIBUTE_INFORMATION			1005
#define SMB_QFS_QUOTA_INFORMATION			1006
#define SMB_QFS_FULL_SIZE_INFORMATION			1007
#define SMB_QFS_OBJECTID_INFORMATION			1008
#define SMB_QFS_SECTOR_SIZE_INFORMATION			1011


/* trans2 qfileinfo/qpathinfo */
/* w2k3 TRANS2ALIASES:
Checking for QPATHINFO aliases
setting up complex file \qpathinfo_aliases.txt
        Found level    1 (0x001) of size  22 (0x16)
        Found level    2 (0x002) of size  26 (0x1a)
        Found level    4 (0x004) of size  41 (0x29)
        Found level    6 (0x006) of size   0 (0x00)
        Found level  257 (0x101) of size  40 (0x28)
        Found level  258 (0x102) of size  24 (0x18)
        Found level  259 (0x103) of size   4 (0x04)
        Found level  260 (0x104) of size  48 (0x30)
        Found level  263 (0x107) of size 126 (0x7e)
        Found level  264 (0x108) of size  28 (0x1c)
        Found level  265 (0x109) of size  38 (0x26)
        Found level  267 (0x10b) of size  16 (0x10)
        Found level 1004 (0x3ec) of size  40 (0x28)
        Found level 1005 (0x3ed) of size  24 (0x18)
        Found level 1006 (0x3ee) of size   8 (0x08)
        Found level 1007 (0x3ef) of size   4 (0x04)
        Found level 1008 (0x3f0) of size   4 (0x04)
        Found level 1009 (0x3f1) of size  48 (0x30)
        Found level 1014 (0x3f6) of size   8 (0x08)
        Found level 1016 (0x3f8) of size   4 (0x04)
        Found level 1017 (0x3f9) of size   4 (0x04)
        Found level 1018 (0x3fa) of size 126 (0x7e)
        Found level 1021 (0x3fd) of size  28 (0x1c)
        Found level 1022 (0x3fe) of size  38 (0x26)
        Found level 1028 (0x404) of size  16 (0x10)
        Found level 1034 (0x40a) of size  56 (0x38)
        Found level 1035 (0x40b) of size   8 (0x08)
Found 27 levels with success status
        Level 267 (0x10b) and level 1028 (0x404) are possible aliases
        Level 265 (0x109) and level 1022 (0x3fe) are possible aliases
        Level 264 (0x108) and level 1021 (0x3fd) are possible aliases
        Level 263 (0x107) and level 1018 (0x3fa) are possible aliases
        Level 260 (0x104) and level 1009 (0x3f1) are possible aliases
        Level 259 (0x103) and level 1007 (0x3ef) are possible aliases
        Level 258 (0x102) and level 1005 (0x3ed) are possible aliases
        Level 257 (0x101) and level 1004 (0x3ec) are possible aliases
Found 8 aliased levels
*/
#define SMB_QFILEINFO_STANDARD                             1
#define SMB_QFILEINFO_EA_SIZE                              2
#define SMB_QFILEINFO_EA_LIST                              3
#define SMB_QFILEINFO_ALL_EAS                              4
#define SMB_QFILEINFO_IS_NAME_VALID                        6  /* only for QPATHINFO */
#define SMB_QFILEINFO_BASIC_INFO	               0x101
#define SMB_QFILEINFO_STANDARD_INFO	               0x102
#define SMB_QFILEINFO_EA_INFO		               0x103
#define SMB_QFILEINFO_NAME_INFO	                       0x104
#define SMB_QFILEINFO_ALL_INFO		               0x107
#define SMB_QFILEINFO_ALT_NAME_INFO	               0x108
#define SMB_QFILEINFO_STREAM_INFO	               0x109
#define SMB_QFILEINFO_COMPRESSION_INFO                 0x10b
#define SMB_QFILEINFO_UNIX_BASIC                       0x200
#define SMB_QFILEINFO_UNIX_LINK                        0x201
#define SMB_QFILEINFO_UNIX_INFO2                       0x20b
#define SMB_QFILEINFO_BASIC_INFORMATION			1004
#define SMB_QFILEINFO_STANDARD_INFORMATION		1005
#define SMB_QFILEINFO_INTERNAL_INFORMATION		1006
#define SMB_QFILEINFO_EA_INFORMATION			1007
#define SMB_QFILEINFO_ACCESS_INFORMATION		1008
#define SMB_QFILEINFO_NAME_INFORMATION			1009
#define SMB_QFILEINFO_POSITION_INFORMATION		1014
#define SMB_QFILEINFO_MODE_INFORMATION			1016
#define SMB_QFILEINFO_ALIGNMENT_INFORMATION		1017
#define SMB_QFILEINFO_ALL_INFORMATION			1018
#define SMB_QFILEINFO_ALT_NAME_INFORMATION	        1021
#define SMB_QFILEINFO_STREAM_INFORMATION		1022
#define SMB_QFILEINFO_COMPRESSION_INFORMATION		1028
#define SMB_QFILEINFO_NETWORK_OPEN_INFORMATION		1034
#define SMB_QFILEINFO_ATTRIBUTE_TAG_INFORMATION		1035



/* trans2 setfileinfo/setpathinfo levels */
/*
w2k3 TRANS2ALIASES
Checking for SETFILEINFO aliases
setting up complex file \setfileinfo_aliases.txt
        Found level    1 (0x001) of size   2 (0x02)
        Found level    2 (0x002) of size   2 (0x02)
        Found level  257 (0x101) of size  40 (0x28)
        Found level  258 (0x102) of size   2 (0x02)
        Found level  259 (0x103) of size   8 (0x08)
        Found level  260 (0x104) of size   8 (0x08)
        Found level 1004 (0x3ec) of size  40 (0x28)
        Found level 1010 (0x3f2) of size   2 (0x02)
        Found level 1013 (0x3f5) of size   2 (0x02)
        Found level 1014 (0x3f6) of size   8 (0x08)
        Found level 1016 (0x3f8) of size   4 (0x04)
        Found level 1019 (0x3fb) of size   8 (0x08)
        Found level 1020 (0x3fc) of size   8 (0x08)
        Found level 1023 (0x3ff) of size   8 (0x08)
        Found level 1025 (0x401) of size  16 (0x10)
        Found level 1029 (0x405) of size  72 (0x48)
        Found level 1032 (0x408) of size  56 (0x38)
        Found level 1039 (0x40f) of size   8 (0x08)
        Found level 1040 (0x410) of size   8 (0x08)
Found 19 valid levels

Checking for SETPATHINFO aliases
        Found level 1004 (0x3ec) of size  40 (0x28)
        Found level 1010 (0x3f2) of size   2 (0x02)
        Found level 1013 (0x3f5) of size   2 (0x02)
        Found level 1014 (0x3f6) of size   8 (0x08)
        Found level 1016 (0x3f8) of size   4 (0x04)
        Found level 1019 (0x3fb) of size   8 (0x08)
        Found level 1020 (0x3fc) of size   8 (0x08)
        Found level 1023 (0x3ff) of size   8 (0x08)
        Found level 1025 (0x401) of size  16 (0x10)
        Found level 1029 (0x405) of size  72 (0x48)
        Found level 1032 (0x408) of size  56 (0x38)
        Found level 1039 (0x40f) of size   8 (0x08)
        Found level 1040 (0x410) of size   8 (0x08)
Found 13 valid levels
*/
#define SMB_SFILEINFO_STANDARD                             1
#define SMB_SFILEINFO_EA_SET                               2
#define SMB_SFILEINFO_BASIC_INFO	               0x101
#define SMB_SFILEINFO_DISPOSITION_INFO		       0x102
#define SMB_SFILEINFO_ALLOCATION_INFO                  0x103
#define SMB_SFILEINFO_END_OF_FILE_INFO                 0x104
#define SMB_SFILEINFO_UNIX_BASIC                       0x200
#define SMB_SFILEINFO_UNIX_LINK                        0x201
#define SMB_SPATHINFO_UNIX_HLINK                       0x203
#define SMB_SPATHINFO_POSIX_ACL                        0x204
#define SMB_SPATHINFO_XATTR                            0x205
#define SMB_SFILEINFO_ATTR_FLAGS                       0x206	
#define SMB_SFILEINFO_UNIX_INFO2                       0x20b
#define SMB_SFILEINFO_BASIC_INFORMATION			1004
#define SMB_SFILEINFO_RENAME_INFORMATION		1010
#define SMB_SFILEINFO_LINK_INFORMATION			1011
#define SMB_SFILEINFO_DISPOSITION_INFORMATION		1013
#define SMB_SFILEINFO_POSITION_INFORMATION		1014
#define SMB_SFILEINFO_FULL_EA_INFORMATION		1015
#define SMB_SFILEINFO_MODE_INFORMATION			1016
#define SMB_SFILEINFO_ALLOCATION_INFORMATION		1019
#define SMB_SFILEINFO_END_OF_FILE_INFORMATION		1020
#define SMB_SFILEINFO_PIPE_INFORMATION			1023
#define SMB_SFILEINFO_VALID_DATA_INFORMATION		1039
#define SMB_SFILEINFO_SHORT_NAME_INFORMATION		1040

/* filemon shows FilePipeRemoteInformation */
#define SMB_SFILEINFO_1025				1025

/* vista scan responds */
#define SMB_SFILEINFO_1027				1027

/* filemon shows CopyOnWriteInformation */
#define SMB_SFILEINFO_1029				1029

/* filemon shows OleClassIdInformation */
#define SMB_SFILEINFO_1032				1032

/* vista scan responds to these */
#define SMB_SFILEINFO_1030				1030
#define SMB_SFILEINFO_1031				1031
#define SMB_SFILEINFO_1036				1036
#define SMB_SFILEINFO_1041				1041
#define SMB_SFILEINFO_1042				1042
#define SMB_SFILEINFO_1043				1043
#define SMB_SFILEINFO_1044				1044

/* trans2 findfirst levels */
/*
w2k3 TRANS2ALIASES:
Checking for FINDFIRST aliases
        Found level    1 (0x001) of size  68 (0x44)
        Found level    2 (0x002) of size  70 (0x46)
        Found level  257 (0x101) of size 108 (0x6c)
        Found level  258 (0x102) of size 116 (0x74)
        Found level  259 (0x103) of size  60 (0x3c)
        Found level  260 (0x104) of size 140 (0x8c)
        Found level  261 (0x105) of size 124 (0x7c)
        Found level  262 (0x106) of size 148 (0x94)
Found 8 levels with success status
Found 0 aliased levels
*/
#define SMB_FIND_STANDARD		    1
#define SMB_FIND_EA_SIZE		    2
#define SMB_FIND_EA_LIST		    3
#define SMB_FIND_DIRECTORY_INFO		0x101
#define SMB_FIND_FULL_DIRECTORY_INFO	0x102
#define SMB_FIND_NAME_INFO		0x103
#define SMB_FIND_BOTH_DIRECTORY_INFO	0x104
#define SMB_FIND_ID_FULL_DIRECTORY_INFO	0x105
#define SMB_FIND_ID_BOTH_DIRECTORY_INFO 0x106
#define SMB_FIND_UNIX_INFO              0x202
#define SMB_FIND_UNIX_INFO2             0x20b

/* flags on trans2 findfirst/findnext that control search */
#define FLAG_TRANS2_FIND_CLOSE          0x1
#define FLAG_TRANS2_FIND_CLOSE_IF_END   0x2
#define FLAG_TRANS2_FIND_REQUIRE_RESUME 0x4
#define FLAG_TRANS2_FIND_CONTINUE       0x8
#define FLAG_TRANS2_FIND_BACKUP_INTENT  0x10

/*
 * DeviceType and Characteristics returned in a
 * SMB_QFS_DEVICE_INFO call.
 */
#define QFS_DEVICETYPE_CD_ROM		        0x2
#define QFS_DEVICETYPE_CD_ROM_FILE_SYSTEM	0x3
#define QFS_DEVICETYPE_DISK			0x7
#define QFS_DEVICETYPE_DISK_FILE_SYSTEM	        0x8
#define QFS_DEVICETYPE_FILE_SYSTEM		0x9

/* Characteristics. */
#define QFS_TYPE_REMOVABLE_MEDIA		0x1
#define QFS_TYPE_READ_ONLY_DEVICE		0x2
#define QFS_TYPE_FLOPPY			        0x4
#define QFS_TYPE_WORM			        0x8
#define QFS_TYPE_REMOTE			        0x10
#define QFS_TYPE_MOUNTED			0x20
#define QFS_TYPE_VIRTUAL			0x40

/* SMB_QFS_SECTOR_SIZE_INFORMATION values */
#define QFS_SSINFO_FLAGS_ALIGNED_DEVICE			0x00000001
#define QFS_SSINFO_FLAGS_PARTITION_ALIGNED_ON_DEVICE	0x00000002
#define QFS_SSINFO_FLAGS_NO_SEEK_PENALTY		0x00000004
#define QFS_SSINFO_FLAGS_TRIM_ENABLED			0x00000008

#define QFS_SSINFO_OFFSET_UNKNOWN			0xffffffff

/*
 * Thursby MAC extensions....
 */

/*
 * MAC CIFS Extensions have the range 0x300 - 0x2FF reserved.
 * Supposedly Microsoft have agreed to this.
 */

#define MIN_MAC_INFO_LEVEL                      0x300
#define MAX_MAC_INFO_LEVEL                      0x3FF
#define SMB_QFS_MAC_FS_INFO                     0x301

#endif
