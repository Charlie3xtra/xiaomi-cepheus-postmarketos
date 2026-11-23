#define _GNU_SOURCE
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
static int call(int fd,unsigned long req,void *p){int r;do{r=ioctl(fd,req,p);}while(r<0&&errno==EINTR);if(r<0){perror("ioctl");exit(2);}return r;}
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
int main(int argc,char **argv){
 if(argc!=3)return 2;
 unsigned target=atoi(argv[1]);int ref_fd=open(argv[2],O_RDONLY);struct stat st;if(ref_fd<0||fstat(ref_fd,&st))return 2;
 size_t len=st.st_size;unsigned char *ref=mmap(NULL,len,PROT_READ,MAP_PRIVATE,ref_fd,0);if(ref==MAP_FAILED)return 2;
 int fd=open("/dev/video0",O_RDWR|O_NONBLOCK);if(fd<0)return 2;
 enum v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
 struct v4l2_requestbuffers req={.count=4,.type=type,.memory=V4L2_MEMORY_MMAP};call(fd,VIDIOC_REQBUFS,&req);
 void *maps[8];size_t sizes[8];if(req.count>8||req.count<2)return 2;
 for(unsigned i=0;i<req.count;i++){
  struct v4l2_plane p[VIDEO_MAX_PLANES]={0};struct v4l2_buffer b={.index=i,.type=type,.memory=V4L2_MEMORY_MMAP,.length=VIDEO_MAX_PLANES,.m.planes=p};call(fd,VIDIOC_QUERYBUF,&b);
  sizes[i]=p[0].length;if(sizes[i]<len)return 2;maps[i]=mmap(NULL,sizes[i],PROT_READ|PROT_WRITE,MAP_SHARED,fd,p[0].m.mem_offset);if(maps[i]==MAP_FAILED)return 2;
  call(fd,VIDIOC_QBUF,&b);
 }
 call(fd,VIDIOC_STREAMON,&type);double start=now();unsigned count=0,bad=0,gaps=0,prev=0;int error=0;
 while(count<target){
  fd_set fds;FD_ZERO(&fds);FD_SET(fd,&fds);struct timeval tv={.tv_sec=3};int ready=select(fd+1,&fds,NULL,NULL,&tv);if(ready<0&&errno==EINTR)continue;if(ready<=0){fprintf(stderr,"frame timeout\n");error=1;break;}
  struct v4l2_plane p[VIDEO_MAX_PLANES]={0};struct v4l2_buffer b={.type=type,.memory=V4L2_MEMORY_MMAP,.length=VIDEO_MAX_PLANES,.m.planes=p};call(fd,VIDIOC_DQBUF,&b);
  if(count&&b.sequence>prev+1)gaps+=b.sequence-prev-1;
  prev=b.sequence;count++;
  int wrong=p[0].bytesused!=len;
  if(!wrong&&memcmp(maps[b.index],ref,len)){
   for(unsigned y=0;y<3000;y++)if(memcmp((char*)maps[b.index]+y*5008,ref+y*5008,5000)){wrong=1;break;}
  }
  if(wrong){bad++;if(bad<6)printf("bad_frame=%u sequence=%u bytes=%u\n",count,b.sequence,p[0].bytesused);}
  call(fd,VIDIOC_QBUF,&b);
  if(count%300==0){printf("frames=%u bad=%u sequence_gaps=%u seconds=%.3f fps=%.3f\n",count,bad,gaps,now()-start,count/(now()-start));fflush(stdout);}
 }
 call(fd,VIDIOC_STREAMOFF,&type);for(unsigned i=0;i<req.count;i++)munmap(maps[i],sizes[i]);close(fd);
 printf("FINAL frames=%u bad=%u sequence_gaps=%u seconds=%.3f fps=%.3f\n",count,bad,gaps,now()-start,count/(now()-start));return error||bad||count!=target;
}
