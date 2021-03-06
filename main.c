/****************************************************************************
*                   KCW  Implement ext2 file system                         *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

// global variables
MINODE minode[NMINODE];
MINODE *root;

PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start; // disk parameters

#include "util.c"
#include "cd_ls_pwd.c"
#include "link_unlink_symlink.c"
#include "mkdir_creat.c"
#include "rmdir.c"
#include "open_close_lseek.c"
#include "read_cat.c"
#include "write_cp_move.c"


int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
    p->status = FREE;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
  }
}

// load root INODE and set root pointer to it
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2);
}

char *disk = "givendisk";

int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  char line[128], cmd[32], pathname[128], third[128];
 
  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }
  dev = fd;    // fd is the global dev
	//does this mean use dev or fd for calls
  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

	printf("ninodes = %d nblocks = %d\n", ninodes, nblocks);

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

  init();  
  mount_root();
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process
  
  while(1){
	  //currently accept ls, cd, pwd, quit, mkdir, creat, link, unlink, symlink, open, close, lseek, pfd, read, cat, write, cp, mv
	  //missing: symlink from LEVEL 1
	  //working on: 
	  //mkdir has errors, see the source file for more detail
  printf("input command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|open|close]\n[lseek|pfd|read|cat|write|cp|mv|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

   // sscanf(line, "%s %s", cmd, pathname);-------Original scanf
   sscanf(line, "%s %s %s", cmd, pathname, third);
    printf("cmd=%s pathname=%s\ntertiery=%s\n", cmd, pathname, third);
  
    if (strcmp(cmd, "ls")==0)
       ls(pathname);

    else if (strcmp(cmd, "cd")==0)
       chdir(pathname);
   
   else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
   
	else if (strcmp(cmd, "mkdir")==0)
		make_dir(pathname);
	
	else if (strcmp(cmd, "creat")==0)
		creat_file(pathname);
	
	else if (strcmp(cmd, "rmdir")==0)
		remove_dir(pathname);
	//link necessitates using 2 parameters
	//as such, sscanf will be expanded 
	else if(strcmp(cmd,"link")==0)
		link(pathname,third);
	
	else if(strcmp(cmd,"unlink")==0)
		unlink(pathname);
	
	else if(strcmp(cmd,"symlink")==0)
	symlink(pathname, third);
	
	
	
	else if(strcmp(cmd,"open")==0)
		open_file(pathname, third);
	
	else if(strcmp(cmd,"close")==0){
		int temp=atoi(pathname);
		close_file(temp);
	}
	
	else if(strcmp(cmd,"lseek")==0){
		int temp=atoi(pathname);
		int temp2=atoi(third);
		my_lseek(temp, temp2);
	}
	
	else if(strcmp(cmd,"pfd")==0)
	pfd();

	else if(strcmp(cmd,"read")==0)
	read_file();

	else if(strcmp(cmd,"cat")==0)
	cat_file(pathname);
	
	else if(strcmp(cmd,"write")==0)
	write_file();
	
	else if(strcmp(cmd,"cp")==0)
	cp(pathname, third);

	else if(strcmp(cmd,"mv")==0)
	mv(pathname, third);

	
    else if (strcmp(cmd, "quit")==0)
       quit();
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
