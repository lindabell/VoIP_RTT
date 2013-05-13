
#include "WAV.h"
#include "stdio.h"

#include<fcntl.h>
#include <io.h>
#include <sys/stat.h>

s8 riff_chunk[4];
struct RIFF_HEADER_DEF RIFF_header;
struct FMT_BLOCK_DEF fmt_block;
int fd;
int fd2;
int len;
u32 dataLen;
u32 datalen2;
u16 dataBuf[1024];

///////////////////
#include <speex/speex.h>
/*60seconds contain 3000 frames of 20ms, and every 20ms will be encoded into 20bytes so 1min=60000bytes */
#define ENCODED_FRAME_SIZE      20
#define FRAME_SIZE              160


char out_bytes[ENCODED_FRAME_SIZE];		   	//输出字节  即：压缩后输出的数据
void *enc_state;

//参数
int quality = 4; 
int complexity=1; 
int vbr=0; 

static SpeexBits EncoderBits;
volatile s16 Buffer[2][FRAME_SIZE];

///////////
int enh=1;/* SPEEX PARAMETERS, MUST REMAINED UNCHANGED */	 //设置参数
void *dec_state;/* Holds the states of the encoder & the decoder */	 //空指针
static SpeexBits DecoderBits;
////////////////////////////////////////////////////////

#define filename "../Soure/1~10s.wav"
#define filename2 "../Soure/new.wav"

void main(void)
{
	fd=open(filename,O_RDONLY|O_BINARY);
	if(!fd)
	{
		printf("open %s fail\n",filename);
		return ;
	}

	do
	{	
		len=read(fd,riff_chunk,sizeof(riff_chunk));
		if(len<sizeof(riff_chunk))
		{
			printf("read riff chunk fail\n");
			close(fd);
			return ;
		}
		if(strncmp("RIFF",riff_chunk,sizeof(riff_chunk))==0)
		{
			len=read(fd,(void*)((u32)&RIFF_header + sizeof(riff_chunk)),sizeof(struct RIFF_HEADER_DEF) - sizeof(riff_chunk));
			if(strncmp(RIFF_header.riff_format, "WAVE", 4) != 0)
			{
				printf("RIFF format error:%-4s\r\n", RIFF_header.riff_format);
				return;
			}
		}
		else if(strncmp(riff_chunk, "fmt ", sizeof(riff_chunk)) == 0)
		{
			/* read riff format block */
			len = read(fd, (void*)((u32)&fmt_block + sizeof(riff_chunk)),
				sizeof(struct FMT_BLOCK_DEF) - sizeof(riff_chunk));
			if(len != sizeof(struct FMT_BLOCK_DEF) - sizeof(riff_chunk))
			{
				printf("read riff format block fail!\r\n");
				return;
			}

			if(fmt_block.fmt_size != 16)
			{
				char tmp[2];
				read(fd, tmp, fmt_block.fmt_size - 16);
			}

// 			if(fmt_block.wav_format.Channels != 2)
// 			{
// 				printf("[err] only support stereo!\r\n");
// 				return;
// 			}
		}
	}while(strncmp(riff_chunk, "data", 4) != 0);

	len=read(fd,&dataLen,4);
	if(len!=4)
	{
		printf("read data len fail \n");
	}
	//做压缩处理
	//做解压处理
	/* Speex encoding initializations */ 
	speex_bits_init(&EncoderBits);
	enc_state = speex_encoder_init(&speex_nb_mode);			       		//narrowband
	speex_encoder_ctl(enc_state, SPEEX_SET_VBR, &vbr);			   		//variable bit-rate(VBR) disable
	speex_encoder_ctl(enc_state, SPEEX_SET_QUALITY,&quality);		   	//0~10 
	speex_encoder_ctl(enc_state, SPEEX_SET_COMPLEXITY, &complexity); 	//best 
	///////////////////////////////////////////////////////////////////////
	speex_bits_init(&DecoderBits);
	dec_state = speex_decoder_init(&speex_nb_mode);				   //解码初始化
	if(dec_state!=0)
	{
		speex_decoder_ctl(dec_state, SPEEX_SET_ENH, &enh);			   //回音消除
	}
	/////////////////////////////////////////////////////////////

	fd2=open(filename2,O_WRONLY|O_CREAT|O_BINARY|O_TRUNC);
	if(!fd)
	{
		printf("open write file fail\n");
	}
	while (1)
	{
	
// 			len=read(fd,dataBuf,sizeof(dataBuf));
// 			if(dataLen>len)
// 			{
// 				write(fd2,dataBuf,len);
// 				dataLen-=len;
// 				datalen2+=len;
// 			}
// 			else
// 			{
// 				write(fd2,dataBuf,dataLen);
// 				datalen2+=dataLen;
// 				break;
// 			}	

			len=read(fd,&Buffer[0],FRAME_SIZE*2);
			if(len<=0) break;

			datalen2+=len;

			speex_bits_reset(&EncoderBits);
			/* Encode the frame */
			speex_encode_int(enc_state, (spx_int16_t*)Buffer[0], &EncoderBits);	//从IN_Buffer[0]中读出AD采样的数据进行压缩
			/* Copy the bits to an array of char that can be decoded */
			speex_bits_write(&EncoderBits, (char *)out_bytes, ENCODED_FRAME_SIZE);	 //写到编码输出缓冲区
			/////////////////////////////////////////////////////////////////////////////////////////////////
			/* Copy the encoded data into the bit-stream struct */
			speex_bits_read_from(&DecoderBits,out_bytes, ENCODED_FRAME_SIZE);	 //从编码缓冲区读出数据
			/* Decode the data */
			speex_decode_int(dec_state, &DecoderBits,(spx_int16_t*)Buffer[1]);	//解码

			write(fd2,&Buffer[1],FRAME_SIZE*2);

	}
	lseek(fd2,0,SEEK_SET);
	RIFF_header.riff_id[0]='R';
	RIFF_header.riff_id[1]='I';
	RIFF_header.riff_id[2]='F';
	RIFF_header.riff_id[3]='F';
	fmt_block.fmt_id[0]='f';
	fmt_block.fmt_id[1]='m';
	fmt_block.fmt_id[2]='t';
	fmt_block.fmt_id[3]=' ';

	write(fd2,&RIFF_header,sizeof(struct RIFF_HEADER_DEF));
	write(fd2,&fmt_block,sizeof(struct FMT_BLOCK_DEF));
	write(fd2,"data",4);
	write(fd2,&datalen2,4);
	close(fd);
	close(fd2);

}


void _speex_putc(int ch, void *file)
{
  while(1)
  {
  };
}
/**
  * @brief  Ovveride the _speex_fatal function of the speex library
  * @param  None
  * @retval : None
  */
void _speex_fatal(const char *str, const char *file, int line)
{
  while(1)
  {
  };
}
